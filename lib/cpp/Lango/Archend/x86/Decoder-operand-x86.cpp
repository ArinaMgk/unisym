// ASCII C++11 TAB4 LF
// Docutitle: x86 Instruction Decoder Operands
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../inc/cpp/Lango/Archend/x86/Decoder-x86.hpp"
extern "C" {
#include "../../../../../inc/cpp/Lango/Archend/x86/data/regdis.h"
}

#define X86_DECODE_U8(data) (*(uint8*)(data))
#if X86_MEMORY
#define X86_DECODE_U32(data) (*(uint32*)(data))
#else
static uint32 X86DecodeUnsigned32(const uint8* data)
{
	return static_cast<uint32>(data[0]) |
		(static_cast<uint32>(data[1]) << 8) |
		(static_cast<uint32>(data[2]) << 16) |
		(static_cast<uint32>(data[3]) << 24);
}
#define X86_DECODE_U32(data) X86DecodeUnsigned32(data)
#endif
#define X86_DECODE_S8(data) static_cast<int8>(X86_DECODE_U8(data))
#define X86_DECODE_S32(data) static_cast<int32>(X86_DECODE_U32(data))

reg_enum X86DecodeRegister(int32 regflags, int regval, int rex)
{
	if (!(regflags & (REGISTER | REGMEM)))
		return R_zero;

	regflags |= REGISTER;

	if (!(REG_AL & ~regflags)) return R_AL;
	if (!(REG_AX & ~regflags)) return R_AX;
	if (!(REG_EAX & ~regflags)) return R_EAX;
	if (!(REG_RAX & ~regflags)) return R_RAX;
	if (!(REG_DL & ~regflags)) return R_DL;
	if (!(REG_DX & ~regflags)) return R_DX;
	if (!(REG_EDX & ~regflags)) return R_EDX;
	if (!(REG_RDX & ~regflags)) return R_RDX;
	if (!(REG_CL & ~regflags)) return R_CL;
	if (!(REG_CX & ~regflags)) return R_CX;
	if (!(REG_ECX & ~regflags)) return R_ECX;
	if (!(REG_RCX & ~regflags)) return R_RCX;
	if (!(FPU0 & ~regflags)) return R_ST0;
	if (!(XMM0 & ~regflags)) return R_XMM0;
	if (!(YMM0 & ~regflags)) return R_YMM0;
	if (!(REG_CS & ~regflags))
		return regval == 1 ? R_CS : R_zero;
	if (!(REG_DESS & ~regflags))
		return regval == 0 || regval == 2 || regval == 3 ?
			aasm_rd_sreg[regval] : R_zero;
	if (!(REG_FSGS & ~regflags))
		return regval == 4 || regval == 5 ?
			aasm_rd_sreg[regval] : R_zero;
	if (!(REG_SEG67 & ~regflags))
		return regval == 6 || regval == 7 ?
			aasm_rd_sreg[regval] : R_zero;

	if (regval < 0 || regval > 15) return R_zero;

	if (!(REG8 & ~regflags)) {
		return rex & (REX_P | REX_NH) ?
			aasm_rd_reg8_rex[regval] : aasm_rd_reg8[regval];
	}
	if (!(REG16 & ~regflags)) return aasm_rd_reg16[regval];
	if (!(REG32 & ~regflags)) return aasm_rd_reg32[regval];
	if (!(REG64 & ~regflags)) return aasm_rd_reg64[regval];
	if (!(REG_SREG & ~regflags)) return aasm_rd_sreg[regval & 7];
	if (!(REG_CREG & ~regflags)) return aasm_rd_creg[regval];
	if (!(REG_DREG & ~regflags)) return aasm_rd_dreg[regval];
	if (!(REG_TREG & ~regflags))
		return regval > 7 ? R_zero : aasm_rd_treg[regval];
	if (!(FPUREG & ~regflags)) return aasm_rd_fpureg[regval & 7];
	if (!(MMXREG & ~regflags)) return aasm_rd_mmxreg[regval & 7];
	if (!(XMMREG & ~regflags)) return aasm_rd_xmmreg[regval];
	if (!(YMMREG & ~regflags)) return aasm_rd_ymmreg[regval];
	return R_zero;
}

uint8* X86DecodeDrex(uint8* data, const uint8* end, insn* instruction, bool& truncated)
{
	if (data == end) {
		truncated = true;
		return nullptr;
	}
	uint8 drex = *data++;
	operand* destination = &instruction->oprs[instruction->drexdst];

	if ((drex & 8) != ((instruction->rex & REX_OC) ? 8 : 0))
		return nullptr;
	instruction->rex = (instruction->rex & ~7) | (drex & 7);
	destination->segment = SEG_RMREG;
	destination->basereg = static_cast<reg_enum>(drex >> 4);
	return data;
}

uint8* X86DecodeEffectiveAddress(uint8* data, const uint8* end,
	int modrm, int address_size, int segment_size, operand* destination,
	insn* instruction, bool& truncated)
{
	int mod = (modrm >> 6) & 3;
	int rm = modrm & 7;
	int scale;
	int index;
	int base;
	uint8 sib = 0;

	if (mod != 3 && rm == 4 && address_size != 16) {
		if (data == end) {
			truncated = true;
			return nullptr;
		}
		sib = *data++;
	}
	if (instruction->rex & REX_D) {
		data = X86DecodeDrex(data, end, instruction, truncated);
		if (!data) return nullptr;
	}
	int rex = instruction->rex;

	if (mod == 3) {
		destination->basereg = static_cast<reg_enum>(
			rm + (rex & REX_B ? 8 : 0));
		destination->segment |= SEG_RMREG;
		return data;
	}

	destination->disp_size = 0;
	destination->eaflags = 0;

	if (address_size == 16) {
		destination->indexreg = destination->basereg =
			static_cast<reg_enum>(-1);
		destination->scale = 1;
		switch (rm) {
		case 0:
			destination->basereg = R_BX;
			destination->indexreg = R_SI;
			break;
		case 1:
			destination->basereg = R_BX;
			destination->indexreg = R_DI;
			break;
		case 2:
			destination->basereg = R_BP;
			destination->indexreg = R_SI;
			break;
		case 3:
			destination->basereg = R_BP;
			destination->indexreg = R_DI;
			break;
		case 4:
			destination->basereg = R_SI;
			break;
		case 5:
			destination->basereg = R_DI;
			break;
		case 6:
			destination->basereg = R_BP;
			break;
		case 7:
			destination->basereg = R_BX;
			break;
		}
		if (rm == 6 && mod == 0) {
			destination->basereg = static_cast<reg_enum>(-1);
			if (segment_size != 16) destination->disp_size = 16;
			mod = 2;
		}
		switch (mod) {
		case 0:
			destination->segment |= SEG_NODISP;
			break;
		case 1:
			if (data == end) {
				truncated = true;
				return nullptr;
			}
			destination->segment |= SEG_DISP8;
			destination->offset = static_cast<int8>(*data++);
			break;
		case 2:
			if (end - data < 2) {
				truncated = true;
				return nullptr;
			}
			destination->segment |= SEG_DISP16;
			destination->offset = *data++;
			destination->offset |= static_cast<unsigned>(*data++) << 8;
			break;
		}
		return data;
	}

	bool address64 = address_size == 64;
	destination->indexreg = R_none;
	destination->basereg = address64 ?
		aasm_rd_reg64[rm | ((rex & REX_B) ? 8 : 0)] :
		aasm_rd_reg32[rm | ((rex & REX_B) ? 8 : 0)];

	if (rm == 5 && mod == 0) {
		if (segment_size == 64) {
			destination->eaflags |= EAF_REL;
			destination->segment |= SEG_RELATIVE;
			mod = 2;
		}
		if (address_size != 64) destination->disp_size = address_size;
		destination->basereg = R_none;
		mod = 2;
	}

	if (rm == 4) {
		scale = (sib >> 6) & 3;
		index = (sib >> 3) & 7;
		base = sib & 7;
		destination->scale = 1 << scale;
		if (index == 4 && !(rex & REX_X))
			destination->indexreg = R_none;
		else if (address64)
			destination->indexreg =
				aasm_rd_reg64[index | ((rex & REX_X) ? 8 : 0)];
		else
			destination->indexreg =
				aasm_rd_reg32[index | ((rex & REX_X) ? 8 : 0)];

		if (base == 5 && mod == 0) {
			destination->basereg = R_none;
			mod = 2;
		}
		else if (address64)
			destination->basereg =
				aasm_rd_reg64[base | ((rex & REX_B) ? 8 : 0)];
		else
			destination->basereg =
				aasm_rd_reg32[base | ((rex & REX_B) ? 8 : 0)];

		if (segment_size == 16) destination->disp_size = 32;
	}

	switch (mod) {
	case 0:
		destination->segment |= SEG_NODISP;
		break;
	case 1:
		if (data == end) {
			truncated = true;
			return nullptr;
		}
		destination->segment |= SEG_DISP8;
		destination->offset = X86_DECODE_S8(data);
		data++;
		break;
	case 2:
		if (end - data < 4) {
			truncated = true;
			return nullptr;
		}
		destination->segment |= SEG_DISP32;
		destination->offset = X86_DECODE_S32(data);
		data += 4;
		break;
	}
	return data;
}
