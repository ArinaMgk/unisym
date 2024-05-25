// ASCII C99 TAB4 CRLF
// Attribute: Little-Endian(Byte, Bit)
// LastCheck: 20240216
// AllAuthor: @dosconio
// ModuTitle: General Header for x86 CPU

#ifndef _INC_X86
#define _INC_X86

#include "../../floating.h"// the file must have been included; this cheats analizer

enum _CPU_x86_descriptor_type
{
	_Dptr_TSS286_Available = 1,
	_Dptr_LDT = 2,
	_Dptr_TSS286_Busy = 3,
	_Dptr_CallGate286 = 4,
	_Dptr_TaskGate = 5,
	_Dptr_InterruptGate286 = 6,
	_Dptr_TrapGate286 = 7,
	//
	_Dptr_TSS386_Available = 9,
	_Dptr_TSS386_Busy = 0xB,
	_Dptr_CallGate386 = 0xC,
	_Dptr_InterruptGate386 = 0xE,
	_Dptr_TrapGate386 = 0xF,
};
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
	byte DB : 1; // 32-bitmode
	byte granularity : 1; // 4k-times
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

static inline dword DescriptorBaseGet(descriptor_t* desc)
{
	return desc->base_low | (desc->base_middle << 16) | (desc->base_high << 24);
}

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

// lib/asm/x86/inst/ioport.asm
void OUT_b(word Port, byte Data);
word IN_b(word Port);
void OUT_w(word Port, word Data);
word IN_w(word Port);
#define outpi// Out to Port's Pin
#define outpb OUT_b
#define outpw OUT_w
#define innpi// In from Port's Pin
#define innpb IN_b
#define innpw IN_w

// ---- lib/asm/x86/inst/manage.asm ----
void HALT(void);

void InterruptEnable(void);
void InterruptDisable(void);
static inline void enInterrupt(int enable) {
	if (enable)
		InterruptEnable();
	else
		InterruptDisable();
}

void InterruptDTabLoad(void* addr);
dword getCR3();
dword getEflags();
void jmpFar(dword offs, dword selc);//{TODO} JumpFar
void CallFar(dword offs, dword selc);
void returnfar(void);

// ---- lib/asm/x86/inst/interrupt.asm ----
void returni(void);// for C

// ---- lib/asm/x86/inst/stack.asm ----
void pushad(void);
void popad(void);
void pushfd(void);
void popfd(void);

// ---- lib/c/processor/x86/delay.c ----
void delay001s();

#endif
