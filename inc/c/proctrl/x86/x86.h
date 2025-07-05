// ASCII C99 TAB4 CRLF
// Attribute: Little-Endian(Byte, Bit)
// LastCheck: 20240216
// AllAuthor: @dosconio
// ModuTitle: General Header for x86 CPU

#include "../../stdinc.h"
#ifndef _INC_X86
#define _INC_X86

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
#ifdef _INC_CPP
	void setRange(dword addr, dword limit) {
		limit_low = limit;
		limit_high = limit >> 16;
		base_low = addr;
		base_middle = addr >> 16;
		base_high = addr >> 24;
	}
#endif
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
	byte param_count : 5;
	byte zero : 3;
	byte type : 4;
	byte notsys : 1;
	byte DPL : 2;
	byte present : 1;
	word offset_high;
#ifdef _INC_CPP
	void setRange(dword addr, word segsel) {
		offset_low = addr;
		offset_high = addr >> 16;
		selector = segsel;
	}
	// default (1_11_01100_00000000)
	// - zero parameter
	// - ring 3
	void setModeCall(dword addr, word segsel) {
		setRange(addr, segsel);
		param_count = 0;
		zero = 0;
		type = 0b1100;
		notsys = 0;
		DPL = 3;
		present = 1;
	}
#endif
} gate_t;

#ifdef _INC_CPP
extern "C" {
#endif

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

// lib/asm/x86/inst/inst.asm
int setA32(int);

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

void TaskReturn();

// ---- lib/asm/x86/inst/interrupt.asm ----
void _returni(void);// for C

// ---- lib/asm/x86/inst/stack.asm ----
void _pushad(void);
void _popad(void);
void _pushfd(void);
void _popfd(void);

// ---- lib/c/processor/x86/delay.c ----
void delay001s();
void delay001ms();

// ---- ---- VGA

// ---- CRT Control Registers
#define CRT_CR_AR 0x03D4 // Address Register
#define CRT_CR_DR 0x03D5 // Data Register

enum {
	CRT_CDR_HorizonalTotal,
	CRT_CDR_HorizonalDisplayEnd,
	CRT_CDR_HorizonalBlankingStart,
	CRT_CDR_HorizonalBlankingEnd,
	CRT_CDR_HorizonalRetraceStart,
	CRT_CDR_HorizonalRetraceEnd,
	//
	CRT_CDR_VerticalTotal,
	CRT_CDR_Overflow,
	CRT_CDR_PresetRowScan,// 0x08
	CRT_CDR_MaxScanLine,
	CRT_CDR_CursorStart,
	CRT_CDR_CursorEnd,
	CRT_CDR_StartAddressHigh,
	CRT_CDR_StartAddressLow,
	CRT_CDR_CursorLocationHigh,
	CRT_CDR_CursorLocationLow,
	CRT_CDR_VerticalRetraceStart,// 0x10
	CRT_CDR_VerticalRetraceEnd,
	CRT_CDR_DisplayEnd,
	CRT_CDR_Offset,
	CRT_CDR_UnderlineLocation,
	CRT_CDR_VerticalBlankingStart,
	CRT_CDR_VerticalBlankingEnd,
	CRT_CDR_ModeControl,// CRTC Mode Control Register 0x17
	CRT_CDR_LineCompare,
};// CRT Controller Data Registers

#ifdef _INC_CPP
}
#endif
#endif
