// ASCII C99 TAB4 CRLF
// Attribute: Little-Endian(Byte, Bit)
// LastCheck: 20240216
// AllAuthor: @dosconio
// ModuTitle: General Header for x86 CPU

#ifndef _INC_X86
#define _INC_X86

#include "../alice.h"

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

#endif
