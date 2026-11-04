// ASCII DJGPP (Debug MSVC2010) TAB4 CRLF
// Attribute: ArnCovenant Host bin^16+ 386+
// LastCheck: RFX19
// AllAuthor: @dosconio
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

#ifndef _INC_AASM_INST


#define _INC_AASM_INST

#include "../../../../c/instruction.h"
#include "../../../../c/stdinc.h"
#include "../../../../../asm/inc/aasm-typedef.h"
#include "data/_asm_inst.h"
#include "data/regs.h"
#include "data/tokens.h"

#define MAX_OPERANDS 5
typedef uint32_t opflags_t;

#ifdef _INC_CPP
extern "C" {
#endif

/*
 * Note that because segment registers may be used as instruction
 * prefixes, we must ensure the enumerations for prefixes and
 * register names do not overlap.
 */
enum prefixes {			/* instruction prefixes */
	P_none = 0,
	PREFIX_ENUM_START = REG_ENUM_LIMIT,
	P_A16 = PREFIX_ENUM_START, P_A32, P_A64, P_ASP,
	P_LOCK, P_O16, P_O32, P_O64, P_OSP,
	P_REP, P_REPE, P_REPNE, P_REPNZ, P_REPZ,
	P_WAIT,
	PREFIX_ENUM_LIMIT
};
enum special_tokens {
	SPECIAL_ENUM_START = PREFIX_ENUM_LIMIT,
	S_ABS = SPECIAL_ENUM_START,
	S_BYTE, S_DWORD, S_FAR, S_LONG, S_NEAR, S_NOSPLIT,
	S_OWORD, S_QWORD, S_REL, S_SHORT, S_STRICT, S_TO, S_TWORD, S_WORD, S_YWORD,
	SPECIAL_ENUM_LIMIT
};
typedef struct operand {	/* operand to an instruction */
	int32_t type;               /* type of operand */
	int disp_size;              /* 0 means default; 16; 32; 64 */
	enum reg_enum basereg, indexreg; /* address registers */
	int scale;			/* index scale */
	int hintbase;
	enum eval_hint hinttype;    /* hint as to real base register */
	int32_t segment;            /* immediate segment, if needed */
	int64_t offset;             /* any immediate number */
	int32_t wrt;                /* segment base it's relative to */
	int eaflags;                /* special EA flags */
	int opflags;                /* see OPFLAG_* defines below */
} operand;

/* Prefix positions: each type of prefix goes in a specific slot.
   This affects the final ordering of the assembled output, which
   shouldn't matter to the processor, but if you have stylistic
   preferences, you can change this.  REX prefixes are handled
   differently for the time being.

   Note that LOCK and REP are in the same slot.  This is
   an x86 architectural constraint. */
enum prefix_pos {
	PPS_WAIT,			/* WAIT (technically not a prefix!) */
	PPS_LREP,			/* Lock or REP prefix */
	PPS_SEG,			/* Segment override prefix */
	PPS_OSIZE,			/* Operand size prefix */
	PPS_ASIZE,			/* Address size prefix */
	MAXPREFIX			/* Total number of prefix slots */
};
enum ea_flags {			/* special EA flags */
	EAF_BYTEOFFS =  1,          /* force offset part to byte size */
	EAF_WORDOFFS =  2,          /* force offset part to [d]word size */
	EAF_TIMESTWO =  4,          /* really do EAX*2 not EAX+EAX */
	EAF_REL	 =  8,		/* IP-relative addressing */
	EAF_ABS      = 16,		/* non-IP-relative addressing */
	EAF_FSGS	 = 32		/* fs/gs segment override present */
};
enum ccode {			/* condition code names */
	C_A, C_AE, C_B, C_BE, C_C, C_E, C_G, C_GE, C_L, C_LE, C_NA, C_NAE,
	C_NB, C_NBE, C_NC, C_NE, C_NG, C_NGE, C_NL, C_NLE, C_NO, C_NP,
	C_NS, C_NZ, C_O, C_P, C_PE, C_PO, C_S, C_Z,
	C_none = -1
};
typedef struct insn {		/* an instruction itself */
	enum prefixes prefixes[MAXPREFIX]; /* instruction prefixes, if any */
	enum _asm_opcode opcode;         /* the opcode - not just the string */
	enum ccode condition;       /* the condition code, if Jcc/SETcc */
	int operands;               /* how many operands? 0-5 */
	int addr_size;		/* address size */
	operand oprs[MAX_OPERANDS]; /* the operands, defined as above */
	int32_t times;              /* repeat count (TIMES prefix) */
	bool prefix_only;           /* emit prefixes without an opcode */
	bool forw_ref;              /* is there a forward reference? */
	int rex;			/* Special REX Prefix */
	int drexdst;		/* Destination register for DREX/VEX suffix */
	int vex_cm;			/* Class and M field for VEX prefix */
	int vex_wlp;		/* W, P and L information for VEX prefix */
} insn;

/*
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



/*
 * REX flags
 */
#define REX_REAL	0x4f	/* Actual REX prefix bits */
#define REX_B		0x01	/* ModRM r/m extension */
#define REX_X		0x02	/* SIB index extension */
#define REX_R		0x04	/* ModRM reg extension */
#define REX_W		0x08	/* 64-bit operand size */
#define REX_L		0x20	/* Use LOCK prefix instead of REX.R */
#define REX_P		0x40	/* REX prefix present/required */
#define REX_H		0x80	/* High register present, REX forbidden */
#define REX_D		0x0100	/* Instruction uses DREX instead of REX */
#define REX_OC		0x0200	/* DREX suffix has the OC0 bit set */
#define REX_V		0x0400	/* Instruction uses VEX/XOP instead of REX */
#define REX_NH		0x0800	/* Instruction which doesn't use high regs */

/*
 * REX_V "classes" (prefixes which behave like VEX)
 */
enum vex_class {
	RV_VEX		= 0,	/* C4/C5 */
	RV_XOP		= 1	/* 8F */
};


	struct itemplate {
		enum _asm_opcode opcode;		/* the token, passed from "parser.c" */
		int operands;		/* number of operands */
		opflags_t opd[MAX_OPERANDS]; /* bit flags for operand types */
		const uint8_t* code;	/* the code it assembles to */
		uint32_t flags;		/* some flags */
	};

	/* Disassembler table structure */
	/* If n == -1, then p points to another table of 256
	   struct disasm_index, otherwise p points to a list of n
	   struct itemplates to consider. */
	struct disasm_index {
		const void* p;
		int n;
	};



	/* Tables for the assembler and disassembler, respectively */
	extern const struct itemplate* const aasm_instructions[];
	extern const struct disasm_index itable[256];
	extern const struct disasm_index* const itable_vex[2][32][8];

	/* Common table for the byte codes */
	extern const uint8_t aasm_bytecodes[];
	extern const char* const _asm_inst_literal_list[];
	extern const char* const aasm_reg_names[];
	extern const int32_t aasm_reg_flags[];
	extern const int aasm_regvals[];

#ifdef _INC_CPP
}

namespace uni {
	class CodeendInterface;
}

using X86Instruction = insn;
using X86Operand = operand;
using X86InstructionTemplate = itemplate;

uint32 X86CpuId(rostr identifier);
const char* prefix_name(int token);

struct X86Parser {
	static X86Instruction* ParseLine(int pass, char* buffer,
		X86Instruction* result, evalfunc evaluate);
};

int64_t X86InstructionSize(int32_t section, int64_t offset,
	int bits, uint32_t cpu, X86Instruction* instruction);
int64_t X86Assemble(int32_t section, int64_t offset,
	int bits, uint32_t cpu, X86Instruction* instruction,
	uni::CodeendInterface* codeend, ListGen* listgen);
#endif
// signify the end of an itemplate
#define ITEMPLATE_END {-1,-1,{-1,-1,-1},NULL,0}

extern const uint8_t _asm_inst_bytes_list[];
extern struct instruction_t instruction_table[];//{TODO}

//{TEMP}:

// Instruction template flags. These specify which processor targets the instruction is eligible for, whether it is privileged or undocumented, and also specify extra error checking on the matching of the instruction. IF_SM stands for Size Match: any operand whose size is not explicitly specified by the template is `really' intended to be the same size as the first size-specified operand. Non-specification is tolerated in the input instruction, but _wrong_ specification is not. IF_SM2 invokes Size Match on only the first _two_ operands, for three-operand instructions such as SHLD: it implies that the first two operands must match in size, but that the third is required to be _unspecified_. IF_SB invokes Size Byte: operands with unspecified size in the template are really bytes, and so no non-byte specification in the input instruction will be tolerated. IF_SW similarly invokes Size Word, and IF_SD invokes Size Doubleword. (The default state if neither IF_SM nor IF_SM2 is specified is that any operand with unspecified size in the template is required to have unspecified size in the instruction too...)/

#define IF_SM     0x00000001UL  /* size match */
#define IF_SM2    0x00000002UL  /* size match first two operands */
#define IF_SB     0x00000004UL  /* unsized operands can't be non-byte */
#define IF_SW     0x00000008UL  /* unsized operands can't be non-word */
#define IF_SD     0x0000000CUL  /* unsized operands can't be non-dword */
#define IF_SQ     0x00000010UL  /* unsized operands can't be non-qword */
#define IF_SO     0x00000014UL  /* unsized operands can't be non-oword */
#define IF_SY     0x00000018UL  /* unsized operands can't be non-yword */
#define IF_SZ     0x0000001CUL  /* unsized operands must match the bitsize */
#define IF_SMASK  0x0000001CUL  /* mask for unsized argument size */
#define IF_AR0	  0x00000020UL  /* SB, SW, SD applies to argument 0 */
#define IF_AR1	  0x00000040UL  /* SB, SW, SD applies to argument 1 */
#define IF_AR2	  0x00000060UL  /* SB, SW, SD applies to argument 2 */
#define IF_AR3	  0x00000080UL  /* SB, SW, SD applies to argument 3 */
#define IF_ARMASK 0x000000E0UL  /* mask for unsized argument spec */
#define IF_ARSHFT 5		/* LSB in IF_ARMASK */
#define IF_PRIV   0x00000100UL  /* it's a privileged instruction */
#define IF_SMM    0x00000200UL  /* it's only valid in SMM */
#define IF_PROT   0x00000400UL  /* it's protected mode only */
#define IF_NOLONG 0x00000800UL  /* it's not available in long mode */
#define IF_UNDOC  0x00001000UL  /* it's an undocumented instruction */
#define IF_FPU    0x00002000UL  /* it's an FPU instruction */
#define IF_MMX    0x00004000UL  /* it's an MMX instruction */
#define IF_3DNOW  0x00008000UL  /* it's a 3DNow! instruction */
#define IF_SSE    0x00010000UL  /* it's a SSE (KNI, MMX2) instruction */
#define IF_SSE2   0x00020000UL  /* it's a SSE2 instruction */
#define IF_SSE3   0x00040000UL  /* it's a SSE3 (PNI) instruction */
#define IF_VMX	  0x00080000UL  /* it's a VMX instruction */
#define IF_LONG   0x00100000UL	/* long mode instruction */
#define IF_SSSE3  0x00200000UL  /* it's an SSSE3 instruction */
#define IF_SSE4A  0x00400000UL  /* AMD SSE4a */
#define IF_SSE41  0x00800000UL  /* it's an SSE4.1 instruction */
#define IF_SSE42  0x00800000UL  /* HACK NEED TO REORGANIZE THESE BITS */
#define IF_SSE5   0x00800000UL  /* HACK NEED TO REORGANIZE THESE BITS */
#define IF_AVX    0x00800000UL  /* HACK NEED TO REORGANIZE THESE BITS */
#define IF_FMA    0x00800000UL  /* HACK NEED TO REORGANIZE THESE BITS */
#define IF_PMASK  0xFF000000UL  /* the mask for processor types */
#define IF_PLEVEL 0x0F000000UL  /* the mask for processor instr. level */
								/* also the highest possible processor */
#define IF_PFMASK 0xF01FFF00UL  /* the mask for disassembly "prefer" */
#define IF_8086   0x00000000UL  /* 8086 instruction */
#define IF_186    0x01000000UL  /* 186+ instruction */
#define IF_286    0x02000000UL  /* 286+ instruction */
#define IF_386    0x03000000UL  /* 386+ instruction */
#define IF_486    0x04000000UL  /* 486+ instruction */
#define IF_PENT   0x05000000UL  /* Pentium instruction */
#define IF_P6     0x06000000UL  /* P6 instruction */
#define IF_KATMAI 0x07000000UL  /* Katmai instructions */
#define IF_WILLAMETTE 0x08000000UL      /* Willamette instructions */
#define IF_PRESCOTT   0x09000000UL      /* Prescott instructions */
#define IF_X86_64 0x0A000000UL	/* x86-64 instruction (long or legacy mode) */
#define IF_NEHALEM 0x0B000000UL  /* Nehalem instruction */
#define IF_WESTMERE 0x0C000000UL   /* Westmere instruction */
#define IF_SANDYBRIDGE 0x0D000000UL /* Sandy Bridge instruction */
#define IF_FUTURE 0x0E000000UL	/* Future processor (not yet disclosed) */
#define IF_X64	  (IF_LONG|IF_X86_64)
#define IF_IA64   0x0F000000UL  /* IA64 instructions (in x86 mode) */
#define IF_CYRIX  0x10000000UL  /* Cyrix-specific instruction */
#define IF_AMD    0x20000000UL  /* AMD-specific instruction */

#endif
