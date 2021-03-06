#include "tty.h"
#include "stdlib.h"
#include "memory_manager.h"
#include "interrupts.h"
#include "multitasking.h"
#include "floppy.h"
#include "timer.h"
#include "listfs.h"

uint32 find_file (uint32 file_sector_number);

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
		if(!strcmp("runp", buffer))
			run_p();
		else if(!strcmp("ps", buffer))
			show_p();
		else if(!strcmp("read", buffer))
			cmd_read_sect();
		else if(!strcmp("ticks", buffer))
			cmd_get_ticks();
		else if(!strcmp("exec", buffer))
			cmd_exec();
		else if(!strcmp("ls", buffer))
			show_files((uint32)first_file_sector_number);
		else if(!strcmp("find", buffer))
			printf("result: %d\n", find_file((uint32)first_file_sector_number));
		else 
			printf("You typed: %s\n", buffer);
	}
}

void init_floppy() { 
	// set drive 0 as current drive
	flpydsk_set_working_drive(0);

	// install floppy disk to IR 38, uses IRQ 6
	flpydsk_install();
	
	// set DMA buffer to 64k
	flpydsk_set_dma(0x20000);//todo: use physical memory manager
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

void run_p() {
	// char* video_mem = 0xB8000;
	// int rows = 25;
	// int columns = 80;
	// char initChar = '&';
	// while (true) {
	// 	for(int i = 0; i < 25; i++) {
	// 		int offset = i * columns + 10;
	// 		*(video_mem + offset * 2) = initChar++;
	// 	}
	// }
	run_new_process();
}


void show_files (uint32 file_sector_number) {
	listfs_file_header *file_header = get_file_info(file_sector_number);
	
	if(strcmp("first.bin", file_header->name)) 
		printf("name: %s\n", file_header->name);
	else 
		printf("name: ***%s***\n", file_header->name);
	if(CHECK_BIT(file_header->flags, 1))
		printf(" type: directory");
	else
		printf(" type: file\n");
	if(file_header->parent == -1)
		printf(" directory: root\n");
	else
		printf(" directory: not root\n");
	printf(" size: %d bytes\n", (uint32)file_header->size);
	if(file_header->next != -1) {
		printf("\n");
		show_files((uint32)file_header->next);
	}
}

uint32 get_file_data_pointer (uint32 sector_list_sector_number) {
	//we support only 1-sector size files right now
	uint64 *sector_list = flpydsk_read_sector(sector_list_sector_number);
	return (uint32)*sector_list;
}

uint32 find_file (uint32 file_sector_number) {
	listfs_file_header *file_header = get_file_info(file_sector_number);
	
	if(strcmp("first.bin", file_header->name) && file_header->next != -1) 
		return find_file((uint32)file_header->next);
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

	// read sector from disk
	sector = (uint8*)flpydsk_read_sector(sectornum);

	// display sector
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

void cmd_exec() {

	uint32 sectornum = 0;
	char sectornumbuf [4];
	uint8* sector;

	printf ("\nPlease type in the sector number [0 is default] > ");
	in_string(sectornumbuf, sizeof(sectornumbuf));
	sectornum = atoi(sectornumbuf);

	printf("\nSector %d contents:\n\n", sectornum);

	// read sector from disk
	sector = (uint8*)flpydsk_read_sector(sectornum);

	// void *address = 0x12000;
	// goto *address;

	printf("\nDone!\n\n");
}

phyaddr init_paging_tables () {
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

	//fill the first megabyte of first page table
	temp_map_page(first_table_phyaddr);
	uint32 *first_table = (uint32*)TEMP_PAGE;
	uint16 entries_count =  0x100000 / 4096;
	uint32 entry_value = 3; //11b
	for (int i = 0; i < entries_count; i++) {
		first_table[i] = entry_value;
		entry_value += 0x1000;
	}

	//fill last page table
	temp_map_page(last_table_phyaddr);
	uint32 *last_table = (uint32*)TEMP_PAGE;
	entry_value = 0x11000 | 3; // 0x11000 | 11b
	for (int i = 0; i < PAGES_PER_TABLE; i++) {
		last_table[i] = entry_value;
		entry_value += 0x1000;
	}

	//map kernel stack
	//last_table[1020] = 0x4000 | 3; //0x4000 + 11b
	last_table[kernel_index] = 0x3000 | 3; //0x3000 + 11b

	return page_dir;
}

void run_new_process () {
	phyaddr page_dir = init_paging_tables();
	Process *new_process = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, 
		PAGE_PRESENT | PAGE_WRITABLE);

	init_address_space(&new_process->address_space, page_dir);
	
	bool suspend = false;

	new_process->suspend = suspend;
	new_process->thread_count = 0;
	strncpy(new_process->name, "first.asm", sizeof(new_process->name));
	list_append((List*)&process_list, (ListItem*)new_process);

	//0x20000 - first.asm is loaded there
	create_thread(new_process, (void*)0x20000, 1, true, suspend);
}