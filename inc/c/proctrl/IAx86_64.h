// ASCII C99 TAB4 CRLF
// Attribute: Little-Endian(Byte, Bit)
// AllAuthor: @ArinaMgk
// ModuTitle: General Header for x86/x64 CPU
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

// Included by x86.h or x64.h

#include "../stdinc.h"
#ifndef _INC_X86_64
#define _INC_X86_64


// ---- ATX Board

enum PORT_ATX_X64
#ifdef _INC_CPP
	: uint16
#endif
{
	PORT_PCI_CONFIG_ADDR = 0x0CF8,// dword
	PORT_PCI_CONFIG_DATA = 0x0CFC,// dword
};


// ---- Structure Definition

typedef enum _CPU_descriptor_type
{
	_Dptr_TSS286_Available = 1,
	_Dptr_LDT = 2,
	_Dptr_TSS286_Busy = 3,
	_Dptr_CallGate286 = 4,
	_Dptr_TaskGate = 5,
	_Dptr_InterruptGate286 = 6,
	_Dptr_TrapGate286 = 7,
	_Dptr_TSS386_Available = 9,
	_Dptr_TSS386_Busy = 0xB,
	_Dptr_CallGate386 = 0xC,
	_Dptr_InterruptGate386 = 0xE,
	_Dptr_TrapGate386 = 0xF,
} _CPU_descriptor_type;

typedef _PACKED(union) _CPU_descriptor
{
	uint64 _data;
	_PACKED(struct) {
		word limit_low;
		word base_low;
		byte base_middle;
		_CPU_descriptor_type typ : 4;
		byte notsys : 1;
		byte DPL : 2;
		byte present : 1;
		byte limit_high : 4;
		byte available : 1;
		byte Mod64 : 1;// only for 64-bitmode code segment
		byte DB : 1; // 32-bitmode, reset if in Mod64 Code
		byte granularity : 1; // 4k-times, not for 64-bitmode code/data
		byte base_high;
	};
	// 64-bitmode: base and limit is useless, the value can be any number
	#ifdef _INC_CPP
	stduint constexpr getAddress(void) {
	    return _IMM(base_low) | (_IMM(base_middle) << 16) | (_IMM(base_high) << 24);
	}
	void setRange(dword addr, dword limit) {
		limit_low = limit;
		limit_high = limit >> 16;
		base_low = addr;
		base_middle = addr >> 16;
		base_high = addr >> 24;
	}
	#endif
} descriptor_t;

_PACKED(struct) _CPU_descriptor64 {
	union _CPU_descriptor base;
	uint32 offset_extn;
	uint32 reserved;
};

#if __BITS__ == 64
#define _CPU_descriptor_tss struct _CPU_descriptor64
#else
#define _CPU_descriptor_tss union _CPU_descriptor
#endif

typedef _PACKED(struct) _CPU_gate_type
{
	word offset_low;
	word selector;
	#if __BITS__ == 32
	byte param_count : 5;
	byte zero : 3;
	#elif __BITS__ == 64
	byte interrupt_stack_table : 3;
	byte zero : 5;
	#endif
	_CPU_descriptor_type type : 4;
	byte notsys : 1;
	byte DPL : 2;// descriptor privilege level
	byte present : 1;
	word offset_high;
	#if __BITS__ == 64
	uint32 offset_extn;
	uint32 reserved;
	#endif
	#ifdef _INC_CPP

	stduint constexpr getAddress(void) {
		return
			#if __BITS__ == 64
			_IMM(offset_extn) << 32 |
			#endif
			_IMM(offset_high) << 16 |
			_IMM(offset_low);
	}

	void setRange(stduint addr, word segsel) {
		offset_low = addr;
		offset_high = addr >> 16;
		#if __BITS__ == 64
		offset_extn = addr >> 32;
		#endif
		selector = segsel;
	}

	// default: zero parameter, ring 3
	void setModeCall(stduint addr, word segsel) {
		setRange(addr, segsel);
		#if __BITS__ == 32
		param_count = 0;
		#elif __BITS__ == 64
		interrupt_stack_table = 0;
		#endif
		zero = 0;
		type = _Dptr_CallGate386;
		notsys = 0;
		DPL = 3;
		present = 1;
	}

	// Make a Interupt Gate with Ring 0
	_CPU_gate_type* setModeRupt(stduint addr, word segm)
	{
		auto gate = this;
		setRange(addr, segm);
		#if __BITS__ == 32
		gate->param_count = 0;
		#elif __BITS__ == 64
		gate->interrupt_stack_table = 0;
		#endif
		gate->zero = 0;
		gate->type = _Dptr_InterruptGate386;
		gate->notsys = 0;
		gate->DPL = 0;
		gate->present = 1;
		return gate;
	}
	

	#endif
} gate_t;

// ---- ---- Registers

typedef struct {
    uint32_t CF  : 1;  // Carry Flag — bit 0
    uint32_t _r1 : 1;  // reserved (always 1 in EFLAGS low-word for FLAGS) – bit 1
    uint32_t PF  : 1;  // Parity Flag — bit 2
    uint32_t _r3 : 1;  // reserved — bit 3
    uint32_t AF  : 1;  // Auxiliary / Adjust Flag — bit 4
    uint32_t _r5 : 1;  // reserved — bit 5
    uint32_t ZF  : 1;  // Zero Flag — bit 6
    uint32_t SF  : 1;  // Sign Flag — bit 7
    uint32_t TF  : 1;  // Trap Flag — bit 8
    uint32_t IF  : 1;  // Interrupt enable Flag — bit 9
    uint32_t DF  : 1;  // Direction Flag — bit 10
    uint32_t OF  : 1;  // Overflow Flag — bit 11

    uint32_t IOPL: 2;  // I/O Privilege Level — bits 12–13
    uint32_t NT  : 1;  // Nested Task — bit 14

    uint32_t _r15 : 1; // reserved — bit 15

    uint32_t RF  : 1;  // Resume Flag — bit 16
    uint32_t VM  : 1;  // Virtual-8086 Mode flag — bit 17
    uint32_t AC  : 1;  // Alignment Check / Access Control — bit 18 (486+)  
    uint32_t VIF : 1;  // Virtual Interrupt Flag — bit 19  
    uint32_t VIP : 1;  // Virtual Interrupt Pending — bit 20  
    uint32_t ID  : 1;  // ID flag — bit 21

    uint32_t _reserved : 10; // bits 22–31: reserved / unused / future
} REG_FLAG_t;


// ---- Calling Convention

struct InterruptFrame {
	stduint ip;
	stduint cs;
	stduint flags;
	stduint sp;
	stduint ss;
};

// -- lib/asm/*/inst/ioport.asm

_ESYM_C void OUT_b(uint16 port, uint8 data);
_ESYM_C uint32 IN_b(uint16 port);
_ESYM_C void OUT_w(uint16 port, uint16 data);
_ESYM_C uint32 IN_w(uint16 port);
_ESYM_C void OUT_d(uint16 port, uint32 data);
_ESYM_C uint32 IN_d(uint16 port);

#define outpi// Out to Port's Pin
#define outpb OUT_b
#define outpw OUT_w
#define outpd OUT_d
#define innpi// In from Port's Pin
#define innpb IN_b
#define innpw IN_w
#define innpd IN_d

void IN_wn(word Port, word* Data, unsigned n_bytes);
void OUT_wn(word Port, word* Data, unsigned n);

// -- lib/asm/*/inst/manage.asm

void HALT(void);

void InterruptEnable(void);
void InterruptDisable(void);
static inline void enInterrupt(int enable) {
	if (enable)
		InterruptEnable();
	else
		InterruptDisable();
}

// Load Series
void InterruptDTabLoad(void* addr);
inline static void loadGDT(stduint address, uint16 length) {
	_PACKED(struct) { uint16 u_16fore; stduint u_xxback; } tmpxx_le;// 6B for 32, 10B for 64
	tmpxx_le.u_xxback = address;
	tmpxx_le.u_16fore = length;
	__asm("lgdt %0" : "=m" (tmpxx_le));
}
// @param length: real number of bytes minus one.
inline static void loadIDT(stduint address, uint16 length) {
	_PACKED(struct) { uint16 u_16fore; stduint u_xxback; } tmpxx_le;
	tmpxx_le.u_xxback = address;
	tmpxx_le.u_16fore = length;
	InterruptDTabLoad(&tmpxx_le);
}

inline static void loadTask(stduint sel) {
	__asm("mov  %0, %%eax" : : "m"(sel));
	__asm("ltr  %ax");
}

// [CR0]
stduint getCR0();

// [CR1]
// stduint getCR1();

// [CR2]
stduint getCR2();

// [CR3]
stduint getCR3();
#if __BITS__ == 32
inline static
void setCR3(stduint cr3)
{
	_ASM volatile("movl %0, %%eax\n" : : "r"(cr3));
	_ASM volatile("movl %eax, %cr3\n");
}
#elif __BITS__ == 64
inline static
void setCR3(stduint cr3)
{
	_ASM volatile("movq %0, %%rax\n" : : "r"(cr3));
	_ASM volatile("movq %%rax, %%cr3\n" : : : "memory");
}
#endif

// SPE [xFLAG]
stduint getFlags();

// SEG [CS]
uint16 getCS(void);//{TODO} x86

// GEN [A]
stduint setA(stduint);// eax/rax

// ---- ---- JMPs ---- ---- //

void jmpFar(stduint offs, stduint selc);


// ---- ---- SYSMAN ---- ---- //

void EnableSSE();

unsigned IfSupport_IA32E();

#endif
