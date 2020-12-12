#include "stdlib.h"
#include "memory_manager.h"
#include "interrupts.h"
#include "multitasking.h"

TSS *tss = (void*)(USER_MEMORY_END - PAGE_SIZE * 3 + 1);

bool multitasking_enabled = false;

void init_multitasking() {
	list_init(&process_list);
	list_init(&thread_list);
	kernel_process = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);
	init_address_space(&kernel_process->address_space, kernel_page_dir);
	alloc_virt_pages(&kernel_process->address_space, tss, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);
	tss->esp0 = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
	tss->ss0 = 16;
	tss->io_map_offset = (uint32)((uint32)tss->io_map - (uint32)tss);
	kernel_process->suspend = false;
	kernel_process->thread_count = 1;
	strncpy(kernel_process->name, "Kernel", sizeof(kernel_process->name));
	list_append((List*)&process_list, (ListItem*)kernel_process);
	kernel_thread = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);
	kernel_thread->process = kernel_process;
	kernel_thread->suspend = false;
	kernel_thread->stack_size = PAGE_SIZE;
	list_append((List*)&thread_list, (ListItem*)kernel_thread);
	current_process = kernel_process;
	current_thread = kernel_thread;
	asm("ltr %w0"::"a"(24));
	multitasking_enabled = true;
}

// IRQ_HANDLER(task_switch_int_handler) {
// 	asm("movl %%esp, %0":"=a"(current_thread->stack_pointer));
// 	do {
// 		current_thread = (Thread*)current_thread->list_item.next;
// 		current_process = current_thread->process;
// 	} while (current_thread->suspend || current_process->suspend);
// 	asm("movl %0, %%cr3"::"a"(current_process->address_space.page_dir));
// 	asm("movl %0, %%esp"::"a"(current_thread->stack_pointer));
// }

void switch_task(Registers *regs) {
	if (multitasking_enabled) {
		memcpy(&current_thread->state, regs, sizeof(Registers));
		do {
			current_thread = (Thread*)current_thread->list_item.next;
			current_process = current_thread->process;
		} while (current_thread->suspend || current_process->suspend);
		asm("movl %0, %%cr3"::"a"(current_process->address_space.page_dir));
		memcpy(regs, &current_thread->state, sizeof(Registers));
	}
}

Thread *create_thread(Process *process, void *entry_point, size_t stack_size, bool kernel, bool suspend) {
	Thread *thread = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);
	thread->process = process;
	thread->suspend = suspend;
	thread->stack_size = stack_size;
	thread->stack_base = alloc_virt_pages(&process->address_space, NULL, -1, (stack_size + PAGE_SIZE - 1) & ~PAGE_OFFSET_MASK, PAGE_PRESENT | PAGE_WRITABLE |
		(kernel ? 0 : PAGE_USER));
	memset(&thread->state, 0, sizeof(Registers));
	uint32 data_selector = (kernel ? 16 : 27);
	uint32 code_selector = (kernel ? 8 : 35);
	thread->state.eflags = 0x202;
	thread->state.cs = code_selector;
	thread->state.eip = (uint32)entry_point;
	thread->state.ss = data_selector;
	thread->state.esp = (uint32)thread->stack_base + thread->stack_size;
	thread->state.ds = data_selector;
	thread->state.es = data_selector;
	thread->state.fs = data_selector;
	thread->state.gs = data_selector;
	list_append((List*)&thread_list, (ListItem*)thread);
	process->thread_count++;
	return thread;
} 