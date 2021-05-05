#include "tty.h"
#include "stdlib.h"
#include "memory_manager.h"
#include "dma.h"
#include "interrupts.h"
#include "multitasking.h"
#include "floppy.h"
#include "timer.h"
#include "listfs.h"

void kernel_main(uint8 boot_disk_id, void *memory_map, uint64 first_file_sector_number) {
	init_memory_manager(memory_map);
	init_interrupts();
	init_multitasking();
	init_tty();
	init_floppy();
	init_list_fs(first_file_sector_number);
	set_text_attr(63);
	clear_screen();

	printf("Welcome to SUN OS!\n\n");
	
	printf("Boot disk id ------- %d\n", boot_disk_id);
	printf("Memory map --------- 0x%x\n", memory_map);
	printf("first_file_sector_number: 0x%x!\n\n", first_file_sector_number);

	display_memory_map(memory_map);

	printf("kernel_page_dir = 0x%x\n", kernel_page_dir);
	printf("memory_size = %d MB\n", memory_size / 1024 / 1024);
	printf("get_page_info(kernel_page_dir, 0xB8000) = 0x%x\n\n", get_page_info(kernel_page_dir, (void*)0xB8000));

	int i;
	for (i = 0; i < kernel_address_space.block_count; i++) {
		printf("type = %d, base = 0x%x, length = 0x%x\n", kernel_address_space.blocks[i].type, kernel_address_space.blocks[i].base,
			kernel_address_space.blocks[i].length);
	}
	printf("\n");

	while (true) {
		char buffer[256];
		out_string("Command>");
		in_string(buffer, sizeof(buffer));
		if(!strcmp("run", buffer))
			run_new_process((uint32)first_file_sector_number);
		else if(!strcmp("ps", buffer))
			show_p();
		else if(!strcmp("read", buffer))
			cmd_read_sect();
		else if(!strcmp("ticks", buffer))
			cmd_get_ticks();
		else if(!strcmp("ls", buffer))
			show_files((uint32)first_file_sector_number);
		else if(!strcmp("test", buffer)) {
			// TSS *tss = (void*)(USER_MEMORY_END - PAGE_SIZE * 3 + 1);
			// void *p = alloc_virt_pages(&kernel_process->address_space, tss, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);	
			// if(p == NULL)
			// 	printf("\n\n!!!right!!!\n\n");
			switch_to_user_mode();
   			char *digits = "0123456789ABCDEF\n";
			asm( "movl %0, %%ebx" : : "a" (digits) :); // syscall function parameter
			asm ("movl $0, %eax"); // syscall function number
			asm("int $0x30");//0x20 - irq_base, 16 - handler index, 0x20 + 16 = 0x30
			asm("hlt");//0x20 - irq_base, 16 - handler index, 0x20 + 16 = 0x30
		}
		else 
			printf("You typed: %s\n", buffer);
	}
}

void init_floppy() { 
	// set drive 0 as current drive
	flpydsk_set_working_drive(0);

	// install floppy disk to IR 38, uses IRQ 6
	flpydsk_install();
	
	// set DMA buffer, limit 16mb
	flpydsk_set_dma(TEMP_DMA_BUFFER_ADDR);//todo: use physical memory manager
}

bool is_last_memory_map_entry(struct memory_map_entry *entry);

void display_memory_map(void *memory_map) {
	char* memory_types[] = {
		{"Available"},			//memory_region.type==0
		{"Reserved"},			//memory_region.type==1
		{"ACPI Reclaim"},		//memory_region.type==2
		{"ACPI NVS Memory"}		//memory_region.type==3
	};
	
	struct memory_map_entry *entry = memory_map;
	int map_entry_size = 24; //in bytes
	int region_number = 1;
	while(true) {
		if(is_last_memory_map_entry(entry))
			break;
		
		printf("region %d: start - %u; length (bytes) - %u; type - %d (%s)\n", 
			region_number, 
			(unsigned long)entry->base,
			(unsigned long)entry->length,
			entry->type,
			memory_types[entry->type-1]);
		entry++;
		region_number++;
	}

	printf("\n");
}

bool is_last_memory_map_entry(struct memory_map_entry *entry) {
	bool result = entry->length == 0
		&& entry->length == 0
		&& entry->length == 0
		&& entry->length == 0;
	return result;
}

void show_p() {
	ListItem* process_item = process_list.first;
	for(int i = 0; i < process_list.count; i++) {
		char* process_name = (*((Process*)process_item)).name;
		printf("  process - %s\n", process_name);
		process_item = process_item->next;
	}
}

void show_files (uint32 file_sector_number) {
	listfs_file_header *file_header = get_file_info(file_sector_number);
	
	printf("name: %s\n", file_header->name);

	if(CHECK_BIT(file_header->flags, 1))
		printf(" type: directory");
	else
		printf(" type: file\n");

	if(file_header->parent == LISTFS_INDICATOR_VALUE)
		printf(" directory: root\n");
	else
		printf(" directory: not root\n");

	printf(" size: %d bytes\n", (uint32)file_header->size);
	
	if(file_header->next != LISTFS_INDICATOR_VALUE) {
		printf("\n");
		show_files((uint32)file_header->next);
	}
}

uint32 get_file_data_pointer (uint32 sector_list_sector_number) {
	//we support only 1-sector size files right now, so it's a little bit crazy function
	uint64 *sector_list = flpydsk_read_sector(sector_list_sector_number);
	uint32 result = -1;
	for(int i = 0; ; i++) {
		if(sector_list[i] == LISTFS_INDICATOR_VALUE)
			break;
		result = (uint32)sector_list[i];
	}
	return result;
}

uint32 find_file (uint32 file_sector_number, char* file_name) {
	listfs_file_header *file_header = get_file_info(file_sector_number);
	
	if(strcmp(file_name, file_header->name) && file_header->next != LISTFS_INDICATOR_VALUE) 
		return find_file((uint32)file_header->next, file_name);
	else {
		return get_file_data_pointer((uint32)file_header->data);
	}
}

void cmd_get_ticks() { 
	printf("\nticks: %d\n", get_tick_count());
}

void cmd_read_sect() {
	uint32 sectornum = 0;
	char sectornumbuf [4];
	uint8* sector;

	printf ("\nPlease type in the sector number [0 is default] > ");
	in_string(sectornumbuf, sizeof(sectornumbuf));
	sectornum = atoi(sectornumbuf);

	printf("\nSector %d contents:\n\n", sectornum);

	sector = (uint8*)flpydsk_read_sector(sectornum);

	if (sector != 0) {
		int i = 0;
		for (int c = 0; c < 4; c++) {
			for (int j = 0; j < 128; j++)
				printf("0x%x ", sector[i + j]);
			i += 128;
			printf("\nPress any key to continue...\n");
			
			char buffer[1];
			in_string(buffer, sizeof(buffer));
		}
	}
	else
		printf("\n*** Error reading sector from disk");

	printf("Done.\n");
}

phyaddr init_paging_tables(phyaddr memory_location_start) {
	uint16 kernel_index = 1022;

	phyaddr first_table_phyaddr = alloc_phys_pages(1);
	phyaddr last_table_phyaddr = alloc_phys_pages(1);
	phyaddr kernel_table_phyaddr = 0x03fec000;

	phyaddr page_dir = alloc_phys_pages(1);
	temp_map_page(page_dir);
	memset((void*)TEMP_PAGE, 0, PAGE_SIZE);

	((uint32*)TEMP_PAGE)[0] = first_table_phyaddr | 7; // 7 == 111b
	((uint32*)TEMP_PAGE)[kernel_index] = kernel_table_phyaddr | 7; // 7 == 111b
	((uint32*)TEMP_PAGE)[1023] = last_table_phyaddr | 7; // 7 == 111b

	//fill the FIRST megabyte address space
	temp_map_page(first_table_phyaddr);
	uint32 *first_table = (uint32*)TEMP_PAGE;

	uint32 video_memoty_start = 0xA0000;
	uint32 video_memoty_end = 0xC0000;

	//fill before video memory
	uint32 entry_value = memory_location_start | 3; //3 = 11b
	uint16 entries_count = video_memoty_start / PAGE_SIZE;
	for (int i = 0; i < entries_count; i++) {
		first_table[i] = entry_value;
		entry_value += 0x1000;
	}

	//fill video memory, because we cannot access it in another case
	uint16 prev_entries_count = entries_count;
	uint32 video_memory_entry_value = video_memoty_start | 3; //3 = 11b
	entries_count = prev_entries_count + ((video_memoty_end - video_memoty_start) / PAGE_SIZE);
	for (int i = prev_entries_count; i < entries_count; i++) {
		first_table[i] = video_memory_entry_value;
		video_memory_entry_value += 0x1000;
	}

	//fill after video memory
	// prev_entries_count = entries_count;
	// entries_count = prev_entries_count + ((0x100000 - video_memoty_end) / PAGE_SIZE);
	// entry_value = entry_value + (video_memoty_end - video_memoty_start);
	// for (int i = prev_entries_count; i < entries_count; i++) {
	// 	first_table[i] = entry_value;
	// 	entry_value += 0x1000;
	// }

	//identity mapping second Mb of memory (for testing purposes)
	phyaddr start_second_mb = 0x100000;
	phyaddr end_second_mb = 0x200000;
	int start_index = start_second_mb / PAGE_SIZE;
	entries_count = start_index + ((end_second_mb - start_second_mb) / PAGE_SIZE);
	entry_value = start_second_mb | 3; //3 = 11b
	for (int i = start_index; i < entries_count; i++) {
		first_table[i] = entry_value;
		entry_value += 0x1000;
	}

	//fill last page table
	temp_map_page(last_table_phyaddr);
	uint32 *last_table = (uint32*)TEMP_PAGE;
	entry_value = 0x11000 | 7; // 0x11000 | 11b
	for (int i = 0; i < PAGES_PER_TABLE; i++) {
		last_table[i] = entry_value;
		entry_value += 0x1000;
	}

	//map kernel stack
	//last_table[1020] = 0x4000 | 3; //0x4000 + 11b
	last_table[kernel_index] = 0x3000 | 7; //0x3000 + 11b

	return page_dir;
}

phyaddr alloc_dma_buffer() {
	// phyaddr buffer = alloc_phys_pages_low(1);
	phyaddr buffer = 0x20000;
	flpydsk_set_dma(buffer);
	return buffer;
}

void run_new_process(uint32 file_sector_number) {
	char file_name[256];
	out_string("type bin file name: ");
	in_string(file_name, sizeof(file_name));

	uint32 file_data_sector_number = find_file(file_sector_number, file_name);
	phyaddr process_base = alloc_dma_buffer();
	(uint8*)flpydsk_read_sector(file_data_sector_number);
	
	// phyaddr page_dir = init_paging_tables(0x1ffff);
	phyaddr page_dir = init_paging_tables(process_base - 1);
	Process *new_process = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, 
		PAGE_PRESENT | PAGE_WRITABLE);

	init_address_space(&new_process->address_space, page_dir);
	
	bool suspend = false;

	new_process->suspend = suspend;
	new_process->thread_count = 0;
	strncpy(new_process->name, file_name, sizeof(new_process->name));

	list_append((List*)&process_list, (ListItem*)new_process);

	create_thread(new_process, (void*)1, 1, true, suspend);
}