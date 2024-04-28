// ASCII C99 TAB4 CRLF
// Attribute: Little-Endian(Byte, Bit)
// LastCheck: 20240216
// AllAuthor: @dosconio
// ModuTitle: General Header for x86 CPU

#ifndef _INC_X86
#define _INC_X86

#include "../stdinc.h"
#include "./interface.x86.h"

typedef struct _CPU_x86_descriptor
{
	word limit_low;
	word base_low;
	byte base_middle;
	byte typ : 4;
	byte notsys : 1;
	byte DPL : 2;
	byte present : 1;
	byte limit_high : 4;
	byte available : 1;
	byte Mod64 : 1;
	byte DB : 1;
	byte granularity : 1;
	byte base_high;
} descriptor_t;

// return nothing
static inline void GlobalDescriptor32Set(descriptor_t* gdte, dword base, dword limit, byte typ, byte DPL, byte not_sys, byte db, byte gran)
{
	gdte->limit_low = limit & 0xFFFF;
	gdte->limit_high = (limit >> 16) & 0xF;
	gdte->base_low = base & 0xFFFF;
	gdte->base_middle = (base >> 16) & 0xFF;
	gdte->base_high = (base >> 24) & 0xFF;
	gdte->typ = typ;
	gdte->notsys = not_sys;
	gdte->DPL = DPL;
	gdte->present = 1;
	gdte->DB = db;
	gdte->granularity = gran;
}

typedef struct _CPU_x86_gate
{
	word offset_low;
	word selector;
	byte param_count;
	byte type : 4;
	byte notsys : 1;
	byte DPL : 2;
	byte present : 1;
	word offset_high;
} gate_t;

static inline gate_t* GateStructInterruptR0(gate_t* gate, dword addr, word segm, byte paracnt)
{
	gate->offset_low = addr & 0xFFFF;
	gate->selector = segm;
	gate->param_count = paracnt;
	gate->type = 0xE;// interrupt gate
	gate->notsys = 0;
	gate->DPL = 0;
	gate->present = 1;
	gate->offset_high = (addr >> 16) & 0xFFFF;
	return gate;
}

// lib/c/processor/x86/delay.c
void delay001s();

#endif
