#include "stdlib.h"
#include "memory_manager.h"
#include "interrupts.h"
#include "multitasking.h"

void task_switch_int_handler();

void init_multitasking() {
	list_init(&process_list);
	list_init(&thread_list);
	kernel_process = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);
	kernel_process->address_space.page_dir = kernel_page_dir;
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
	set_int_handler(irq_base, task_switch_int_handler, 0x8E);
}

IRQ_HANDLER(task_switch_int_handler) {
	asm("movl %%esp, %0":"=a"(current_thread->stack_pointer));
	do {
		current_thread = (Thread*)current_thread->list_item.next;
		current_process = current_thread->process;
	} while (current_thread->suspend || current_process->suspend);
	asm("movl %0, %%cr3"::"a"(current_process->address_space.page_dir));
	asm("movl %0, %%esp"::"a"(current_thread->stack_pointer));
}

void switch_task() {
	asm("hlt");
}

Thread *create_thread(Process *process, void *entry_point, size_t stack_size, bool kernel, bool suspend) {
	Thread *thread = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE);
	thread->process = process;
	thread->suspend = suspend;
	// ... вот тут должно быть создание и заполнение стека нити ...
	list_append((List*)&thread_list, (ListItem*)thread);
	process->thread_count++;
	return thread;
} 