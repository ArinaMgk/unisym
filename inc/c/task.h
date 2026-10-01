// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant yo Host
// LastCheck: 20240313
// AllAuthor: @dosconio
// ModuTitle: x86 Task Module
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

#ifndef _INC_TASK
#define _INC_TASK

#include "stdinc.h"

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
// alignas(16)
_PACKED(struct) NormalTaskContext {
	// x64 0x00
	_PACKED(union) {
		_PACKED(struct) {
			stduint AX;
			stduint CX;
			stduint DX;
			stduint BX;
			stduint SP;
			stduint BP;
			stduint SI;
			stduint DI;
		};
		stduint GPR[16];// R0~R15
	};
	// x64 0x80, x86 0x40
	stduint IP;// PC
	stduint FLAG;
	stduint CR3;
	stduint RING;
	// x64 0xA0, x86 0x50
	uint16 CS, DS, ES, SS, FS, GS, _NS0, _NS1;
	// x64 0xB0, x86 0x60
	byte floating_point_context[512];
	// x64 0x2B0
};

#elif defined(_MCCA) && ((_MCCA & 0xFF00) == 0x1000)// RISCV

_PACKED(struct) NormalTaskContext {
	// ignore x0
	stduint ra;   // [0]  x1
	union { stduint sp, SP; };   // [1]  x2
	stduint gp;   // [2]  x3
	stduint tp;   // [3]  x4
	stduint t0;   // [4]  x5
	stduint t1;   // [5]  x6
	stduint t2;   // [6]  x7
	stduint s0;   // [7]  x8
	stduint s1;   // [8]  x9
	stduint a0;   // [9]  x10
	stduint a1;   // [10] x11
	stduint a2;   // [11] x12
	stduint a3;   // [12] x13
	stduint a4;   // [13] x14
	stduint a5;   // [14] x15
	stduint a6;   // [15] x16
	stduint a7;   // [16] x17
	stduint s2;   // [17] x18
	stduint s3;   // [18] x19
	stduint s4;   // [19] x20
	stduint s5;   // [20] x21
	stduint s6;   // [21] x22
	stduint s7;   // [22] x23
	stduint s8;   // [23] x24
	stduint s9;   // [24] x25
	stduint s10;  // [25] x26
	stduint s11;  // [26] x27
	stduint t3;   // [27] x28
	stduint t4;   // [28] x29
	stduint t5;   // [29] x30
	stduint t6;   // [30] x31
	//
	union { stduint mepc, IP; };// [31]
	stduint mstatus;// [32] mret to S/U
	stduint satp;// [33] cr3
	stduint kernel_sp;// [34]
	stduint kernel_ra;// [35] by SwitchTaskContext
	#if __BITS__ == 32 || __BITS__ == 64
	// In RV32, 35 elements * 4 bytes = 140 bytes.
	// Which is not a multiple of 16. The next multiple of 16 (alignment) is 144.
	// stduint _paddings[1];
#endif
};

#elif (defined(_MCCA) && ((_MCCA & 0xFFFF) == 0x2032))

_PACKED(struct) NormalTaskContext {
	// 0x00
	stduint R0, R1, R2, R3, R4, R5, R6, R7;
	stduint R8, R9, R10, R11, R12;
	union { stduint sp, SP, psp; };// 0x34
	union { stduint lr, LR, exc_return; };// 0x38 EXC_RETURN on Cortex-M: 0xFFFFFFFD thread+PSP, 0xFFFFFFF9 thread+MSP
	union { stduint pc, IP; };// 0x3C
	stduint CPSR, SPSR;// 0x40
	stduint CONTROL;// 0x48
	// 0x4C
	stduint SP_svc, LR_svc;
	stduint SP_irq, LR_irq;
	stduint SP_abt, LR_abt;
	stduint SP_und, LR_und;
	stduint SP_fiq, LR_fiq, SPSR_fiq, R8_fiq, R9_fiq, R10_fiq, R11_fiq, R12_fiq;
	// 0x8C
	stduint TTBR0, TTBR1, TTBCR, DACR;
	stduint CONTEXTIDR;// 0x9C
	union {
		uint64 VFP_D[32];// A-profile: D0-D31
		stduint S16_S31[16];// Cortex-M: S16-S31, S0-S15 stay in the hardware frame
	};// 0xA0
	stduint FPSCR, FPEXC;// 0x1A0
	stduint _reserved[2];// 0x1A8
};// 0x1B0

#elif (defined(_MCCA) && ((_MCCA & 0xFFFF) == 0x2064))

// AArch64: paging by ttbr0_el1/ttbr1_el1/tcr_el1, mair_el1 indexes PageEntry.attr_index, contextidr_el1 is the ASID
_PACKED(struct) NormalTaskContext {
	// 0x000, X29 FP and X30 LR
	stduint X0, X1, X2, X3, X4, X5, X6, X7;
	stduint X8, X9, X10, X11, X12, X13, X14, X15;
	stduint X16, X17, X18, X19, X20, X21, X22, X23;
	stduint X24, X25, X26, X27, X28, X29, X30;
	union { stduint sp_el0, SP; };// 0x0F8
	union { stduint sp_el1, kernel_sp; };// 0x100
	union { stduint elr_el1, IP; };// 0x108
	stduint spsr_el1;// 0x110
	stduint ttbr0_el1, ttbr1_el1, tcr_el1;// 0x118
	stduint mair_el1, contextidr_el1;// 0x130
	uint64 q0_q31[64];// 0x140, Q0-Q31 as 64-bit low/high halves
	stduint fpsr, fpcr;// 0x340
};// 0x350


#else

_PACKED(struct) NormalTaskContext { int _; };

#endif

// task.asm
_ESYM_C void SwitchTaskContext(struct NormalTaskContext* nex, struct NormalTaskContext* crt);
_ESYM_C void DirectTaskContext(struct NormalTaskContext* nex);

#if defined(_ARC_x86)
#if _ARC_x86 >= 3

typedef struct TaskStateSegmentx86
{
	word LastTSS;
	word NextTSS;// static, Mecocoa's Design
	dword ESP0;
	word SS0;
	word Padding0;
	dword ESP1;
	word SS1;
	word Padding1;
	dword ESP2;
	word SS2;
	word Padding2;
	union { dword CR3; dword PDBR; }; // Control Register 3, Page Directory Base
	dword EIP;
	dword EFLAGS;
	dword EAX;
	dword ECX;
	dword EDX;
	dword EBX;
	dword ESP;
	dword EBP;
	dword ESI;
	dword EDI;
	word ES;
	word Padding3;
	word CS;
	word Padding4;
	word SS;
	word Padding5;
	word DS;
	word Padding6;
	word FS;
	word Padding7;
	word GS;
	word Padding8;
	word LDTDptr;
	word LDTLength;// static, Mecocoa's Design
	word STRC_15_T;
	word IO_MAP; // (102D) default 103
	// ---- 0104d:
} TSS_t;
// [TSS descriptor]
// Type = 10x1 + L=0 +  D/B=0 + 16B  64-bit TSS
//             + L=1 OR D/B=1        32-bit TSS

typedef struct
{
	word LDTSelector;
	word TSSSelector;
	word parent;
	descriptor_t* LDT;
	TSS_t* TSS;
	byte ring: 2;
	dword esp0, esp1, esp2, esp3;
	dword entry;
	void* IOMap;//{unused} //[Com-Covenant] usually after TSS in memory
	stduint TSSBlockLength;//{unused} from beginning off TSS to end of IOMap
} TaskFlat_t;// for passing paras to func TaskFlatRegister()

#if defined(_INC_CPP)
extern "C" {
#endif

void TaskFlatRegister(TaskFlat_t* TaskFlat, descriptor_t* GDT);
	
#if defined(_INC_CPP)
}
#endif


#endif
#elif defined(_MCCA) && _MCCA == 0x8664

typedef _PACKED(struct) TaskStateSegmentx64
{
	uint32 _reserved0;
	uint64 RSP0;
	uint64 RSP1;
	uint64 RSP2;
	uint64 _reserved1;
	uint64 IST1;
	uint64 IST2;
	uint64 IST3;
	uint64 IST4;
	uint64 IST5;
	uint64 IST6;
	uint64 IST7;
	uint64 _reserved2;
	uint16 _reserved3;
	uint16 IOMapBase;
} TSS_t;


#endif // _ARC_x86
#endif
