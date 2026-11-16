// ASCII C++11 TAB4 LF
// Docutitle: x86 Instruction Decoder Internals
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_X86_DECODER_HPP
#define UNISYM_LANGO_ARCHEND_X86_DECODER_HPP

#include "./Instruction-x86.hpp"

// Flags stored in operand::segment while decoding an instruction.
enum X86DecodeOperandFlag : int {
	SEG_RELATIVE = 1, // Operand contains a relative address.
	SEG_32BIT = 2,   // Operand uses a 32-bit address or displacement.
	SEG_RMREG = 4,   // Operand is encoded by a register field.
	SEG_DISP8 = 8,   // Effective address contains an 8-bit displacement.
	SEG_DISP16 = 16, // Effective address contains a 16-bit displacement.
	SEG_DISP32 = 32, // Effective address contains a 32-bit displacement.
	SEG_NODISP = 64, // Effective address has no encoded displacement.
	SEG_SIGNED = 128, // Immediate value is signed.
	SEG_64BIT = 256, // Operand uses a 64-bit address or displacement.
};

// Prefix state collected before matching an x86 instruction template.
struct X86DecodePrefixes {
	uint8 osize;  // Effective operand size.
	uint8 asize;  // Effective address size.
	uint8 osp;    // Operand-size prefix is present.
	uint8 asp;    // Address-size prefix is present.
	uint8 rep;    // REP or REPNE prefix byte.
	uint8 seg;    // Segment override prefix byte.
	uint8 wait;   // WAIT prefix byte.
	uint8 lock;   // LOCK prefix byte.
	uint8 vex[3]; // Raw VEX prefix bytes.
	uint8 vex_c;  // VEX prefix class.
	uint8 vex_m;  // VEX map field.
	uint8 vex_v;  // VEX source register field.
	uint8 vex_lp; // VEX vector length and mandatory prefix fields.
	uint32 rex;   // REX, DREX, or VEX decoder state.
};

reg_enum X86DecodeRegister(int32 regflags, int regval, int rex);
uint8* X86DecodeDrex(uint8* data, const uint8* end, insn* instruction, bool& truncated);
uint8* X86DecodeEffectiveAddress(uint8* data, const uint8* end,
	int modrm, int address_size, int segment_size, operand* destination,
	insn* instruction, bool& truncated);

#endif
