#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "stdlib.h"

typedef struct {
	uint32 gs, fs, es, ds;
	uint32 ebp, edi, esi, edx, ecx, ebx, eax;
	uint32 eip, cs;
	uint32 eflags;
	uint32 esp, ss;
} Registers;

typedef struct {
	uint32 reserved_1;
	uint32 esp0;
	uint32 ss0;
	uint32 esp1;
	uint32 ss1;
	uint32 esp2;
	uint32 ss2;
	uint32 cr3;
	uint32 eip;
	uint32 eflags;
	uint32 eax;
	uint32 ecx;
	uint32 edx;
	uint32 ebx;
	uint32 esp;
	uint32 ebp;
	uint32 esi;
	uint32 edi;
	uint32 es;
	uint32 cs;
	uint32 ss;
	uint32 ds;
	uint32 fs;
	uint32 gs;
	uint32 ldtr;
	uint16 reserved_2;
	uint16 io_map_offset;
	uint8 io_map[8192 + 1];
} __attribute__((packed)) TSS;

uint8 irq_base;
uint8 irq_count;

#define IRQ_HANDLER(name) void name(); \
	asm("_"#name ": pusha \n call _wrp_" #name " \n movb $0x20, %al \n outb %al, $0x20 \n outb %al, $0xA0 \n popa \n iret"); \
	void wrp_##name()

void init_interrupts();
void set_int_handler(uint8 index, void *handler, uint8 type);

#endif 