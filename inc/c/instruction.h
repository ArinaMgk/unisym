// ASCII CPL TAB4 CRLF
// Docutitle: (Module) Instruction
// Codifiers: @dosconio: 20240426
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
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

#ifndef _INC_Instruction
#define _INC_Instruction

#include "stdinc.h"
#include "../../asm/data/_asm_inst.h"

struct instruction_property_t
{
	enum Architecture_t instset;
};

struct operand_t
{
	stduint typ;
	pureptr_t ptr;
};

struct instruction_t
{
	enum _asm_opcode opcode;//{ISSUE : if instructions over size in the future} 
	byte numof_operands;
	byte bytof_instruction;
	struct operand_t* operands;
	const byte* binary;
	struct instruction_property_t property;
};

_TEMP struct _nasm_instruction_t {
	enum _asm_opcode opcode; 
	byte numof_operands;
	struct operand_t operands[_ASM_OPERANDS_MAXNUM];
	const byte* binary;
	uint32 flags;
};

_TODO inline static void make_inst(struct instruction_t* dst, const struct _nasm_instruction_t* src) {

}

/*{TODO}
 * -----------------------------------------------------------
 * Format of the `insn' structure returned from `parser.c' and
 * passed into `assemble.c'
 * -----------------------------------------------------------
 */

/*
 * Here we define the operand types. These are implemented as bit
 * masks, since some are subsets of others; e.g. AX in a MOV
 * instruction is a special operand type, whereas AX in other
 * contexts is just another 16-bit register. (Also, consider CL in
 * shift instructions, DX in OUT, etc.)
 *
 * The basic concept here is that
 *    (class & ~operand) == 0
 *
 * if and only if "operand" belongs to class type "class".
 *
 * The bits are assigned as follows:
 *
 * Bits 0-7, 23, 29: sizes
 *  0:  8 bits (BYTE)
 *  1: 16 bits (WORD)
 *  2: 32 bits (DWORD)
 *  3: 64 bits (QWORD)
 *  4: 80 bits (TWORD)
 *  5: FAR
 *  6: NEAR
 *  7: SHORT
 * 23: 256 bits (YWORD)
 * 29: 128 bits (OWORD)
 *
 * Bits 8-11 modifiers
 *  8: TO
 *  9: COLON
 * 10: STRICT
 * 11: (reserved)
 *
 * Bits 12-15: type of operand
 * 12: REGISTER
 * 13: IMMEDIATE
 * 14: MEMORY (always has REGMEM attribute as well)
 * 15: REGMEM (valid EA operand)
 *
 * Bits 16-19, 28: subclasses
 * With REG_CDT:
 * 16: REG_CREG (CRx)
 * 17: REG_DREG (DRx)
 * 18: REG_TREG (TRx)

 * With REG_GPR:
 * 16: REG_ACCUM  (AL, AX, EAX, RAX)
 * 17: REG_COUNT  (CL, CX, ECX, RCX)
 * 18: REG_DATA   (DL, DX, EDX, RDX)
 * 19: REG_HIGH   (AH, CH, DH, BH)
 * 28: REG_NOTACC (not REG_ACCUM)
 *
 * With REG_SREG:
 * 16: REG_CS
 * 17: REG_DESS (DS, ES, SS)
 * 18: REG_FSGS
 * 19: REG_SEG67
 *
 * With FPUREG:
 * 16: FPU0
 *
 * With XMMREG:
 * 16: XMM0
 *
 * With YMMREG:
 * 16: YMM0
 *
 * With MEMORY:
 * 16: MEM_OFFS (this is a simple offset)
 * 17: IP_REL (IP-relative offset)
 *
 * With IMMEDIATE:
 * 16: UNITY (1)
 * 17: BYTENESS16 (-128..127)
 * 18: BYTENESS32 (-128..127)
 * 19: BYTENESS64 (-128..127)
 *
 * Bits 20-22, 24-27: register classes
 * 20: REG_CDT (CRx, DRx, TRx)
 * 21: RM_GPR (REG_GPR) (integer register)
 * 22: REG_SREG
 * 24: FPUREG
 * 25: RM_MMX (MMXREG)
 * 26: RM_XMM (XMMREG)
 * 27: RM_YMM (YMMREG)
 *
 * Bit 31 is currently unallocated.
 *
 * 30: SAME_AS
 * Special flag only used in instruction patterns; means this operand
 * has to be identical to another operand.  Currently only supported
 * for registers.
 */

typedef uint32_t opflags_t;

/* Size, and other attributes, of the operand */
#define BITS8		0x00000001U
#define BITS16		0x00000002U
#define BITS32		0x00000004U
#define BITS64		0x00000008U   /* x64 and FPU only */
#define BITS80		0x00000010U   /* FPU only */
#define BITS128		0x20000000U
#define BITS256		0x00800000U
#define FAR		0x00000020U   /* grotty: this means 16:16 or */
									   /* 16:32, like in CALL/JMP */
#define NEAR		0x00000040U
#define SHORT		0x00000080U   /* and this means what it says :) */

#define SIZE_MASK	0x208000FFU   /* all the size attributes */

/* Modifiers */
#define MODIFIER_MASK	0x00000f00U
#define TO		0x00000100U   /* reverse effect in FADD, FSUB &c */
#define COLON		0x00000200U   /* operand is followed by a colon */
#define STRICT		0x00000400U   /* do not optimize this operand */

/* Type of operand: memory reference, register, etc. */
#define OPTYPE_MASK	0x0000f000U
#define REGISTER	0x00001000U   /* register number in 'basereg' */
#define IMMEDIATE	0x00002000U
#define MEMORY		0x0000c000U
#define REGMEM		0x00008000U   /* for r/m, ie EA, operands */

/* Register classes */
#define REG_EA		0x00009000U   /* 'normal' reg, qualifies as EA */
#define RM_GPR		0x00208000U   /* integer operand */
#define REG_GPR		0x00209000U   /* integer register */
#define REG8		0x00209001U   /*  8-bit GPR  */
#define REG16		0x00209002U   /* 16-bit GPR */
#define REG32		0x00209004U   /* 32-bit GPR */
#define REG64		0x00209008U   /* 64-bit GPR */
#define FPUREG		0x01001000U   /* floating point stack registers */
#define FPU0		0x01011000U   /* FPU stack register zero */
#define RM_MMX		0x02008000U   /* MMX operand */
#define MMXREG		0x02009000U   /* MMX register */
#define RM_XMM		0x04008000U   /* XMM (SSE) operand */
#define XMMREG		0x04009000U   /* XMM (SSE) register */
#define XMM0		0x04019000U   /* XMM register zero */
#define RM_YMM		0x08008000U   /* YMM (AVX) operand */
#define YMMREG		0x08009000U   /* YMM (AVX) register */
#define YMM0		0x08019000U   /* YMM register zero */
#define REG_CDT		0x00101004U   /* CRn, DRn and TRn */
#define REG_CREG	0x00111004U   /* CRn */
#define REG_DREG	0x00121004U   /* DRn */
#define REG_TREG	0x00141004U   /* TRn */
#define REG_SREG	0x00401002U   /* any segment register */
#define REG_CS		0x00411002U   /* CS */
#define REG_DESS	0x00421002U   /* DS, ES, SS */
#define REG_FSGS	0x00441002U   /* FS, GS */
#define REG_SEG67	0x00481002U   /* Unimplemented segment registers */

#define REG_RIP		0x00801008U   /* RIP relative addressing */
#define REG_EIP		0x00801004U   /* EIP relative addressing */

/* Special GPRs */
#define REG_SMASK	0x100f0000U   /* a mask for the following */
#define REG_ACCUM	0x00219000U   /* accumulator: AL, AX, EAX, RAX */
#define REG_AL		0x00219001U
#define REG_AX		0x00219002U
#define REG_EAX		0x00219004U
#define REG_RAX		0x00219008U
#define REG_COUNT	0x10229000U   /* counter: CL, CX, ECX, RCX */
#define REG_CL		0x10229001U
#define REG_CX		0x10229002U
#define REG_ECX		0x10229004U
#define REG_RCX		0x10229008U
#define REG_DL		0x10249001U   /* data: DL, DX, EDX, RDX */
#define REG_DX		0x10249002U
#define REG_EDX		0x10249004U
#define REG_RDX		0x10249008U
#define REG_HIGH	0x10289001U   /* high regs: AH, CH, DH, BH */
#define REG_NOTACC	0x10000000U   /* non-accumulator register */
#define REG8NA		0x10209001U   /*  8-bit non-acc GPR  */
#define REG16NA		0x10209002U   /* 16-bit non-acc GPR */
#define REG32NA		0x10209004U   /* 32-bit non-acc GPR */
#define REG64NA		0x10209008U   /* 64-bit non-acc GPR */

/* special types of EAs */
#define MEM_OFFS	0x0001c000U   /* simple [address] offset - absolute! */
#define IP_REL		0x0002c000U   /* IP-relative offset */

/* memory which matches any type of r/m operand */
#define MEMORY_ANY	(MEMORY|RM_GPR|RM_MMX|RM_XMM|RM_YMM)

/* special type of immediate operand */
#define UNITY		0x00012000U   /* for shift/rotate instructions */
#define SBYTE16		0x00022000U   /* for op r16,immediate instrs. */
#define SBYTE32		0x00042000U   /* for op r32,immediate instrs. */
#define SBYTE64		0x00082000U   /* for op r64,immediate instrs. */
#define BYTENESS	0x000e0000U   /* for testing for byteness */

/* special flags */
#define SAME_AS		0x40000000U



#endif
