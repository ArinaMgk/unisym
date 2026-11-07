// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Instruction Encoder
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/riscv/Instruction-riscv.hpp"

namespace uni::lango {

namespace {

bool SignedRange(int64_t value, unsigned bits)
{
	int64_t limit = INT64_C(1) << (bits - 1);
	return value >= -limit && value < limit;
}

bool RequireImmediate(const RISCVValue& value, int64_t& immediate)
{
	if (value.unknown || value.external || value.section != NO_SEG ||
		value.wrt != NO_SEG) {
		aasm_log(_LOG_ERROR, "RISC-V immediate must be an absolute value");
		return false;
	}
	immediate = value.offset;
	return true;
}

bool ResolveTarget(const AssemblySession& session, const RISCVValue& value,
	int64_t offset, int64_t& displacement)
{
	if (value.unknown || value.external || value.wrt != NO_SEG) {
		aasm_log(_LOG_ERROR, "RISC-V target is unresolved");
		return false;
	}
	if (value.section != NO_SEG && value.section != session.Location.segment) {
		aasm_log(_LOG_ERROR,
			"RISC-V cross-section target requires relocation support");
		return false;
	}
	displacement = value.offset - offset;
	return true;
}

uint32_t EncodeR(uint32_t fixed, uint32_t rd, uint32_t rs1, uint32_t rs2)
{
	return fixed | rd << 7 | rs1 << 15 | rs2 << 20;
}

uint32_t EncodeI(uint32_t fixed, uint32_t rd, uint32_t rs1, int64_t immediate)
{
	return fixed | rd << 7 | rs1 << 15 |
		(static_cast<uint32_t>(immediate) & 0xfff) << 20;
}

uint32_t EncodeS(uint32_t fixed, uint32_t rs1, uint32_t rs2,
	int64_t immediate)
{
	uint32_t value = static_cast<uint32_t>(immediate) & 0xfff;
	return fixed | (value & 0x1f) << 7 |
		rs1 << 15 | rs2 << 20 | (value >> 5) << 25;
}

uint32_t EncodeB(uint32_t fixed, uint32_t rs1, uint32_t rs2,
	int64_t immediate)
{
	uint32_t value = static_cast<uint32_t>(immediate) & 0x1fff;
	return fixed | ((value >> 11) & 1) << 7 |
		((value >> 1) & 0xf) << 8 | rs1 << 15 |
		rs2 << 20 | ((value >> 5) & 0x3f) << 25 |
		((value >> 12) & 1) << 31;
}

uint32_t EncodeJ(uint32_t fixed, uint32_t rd, int64_t immediate)
{
	uint32_t value = static_cast<uint32_t>(immediate) & 0x1fffff;
	return fixed | rd << 7 | ((value >> 12) & 0xff) << 12 |
		((value >> 11) & 1) << 20 | ((value >> 1) & 0x3ff) << 21 |
		((value >> 20) & 1) << 31;
}

bool RequireIImmediate(const RISCVOperand& operand, int64_t& immediate)
{
	if (!RequireImmediate(operand.value, immediate)) return false;
	if (!SignedRange(immediate, 12)) {
		aasm_log(_LOG_ERROR, "RISC-V immediate is outside the 12-bit range");
		return false;
	}
	return true;
}

}

bool RISCVEncoder::Encode(const AssemblySession& session,
	const RISCVInstruction& instruction, int64_t offset, uint32_t& encoding)
{
	const RISCVInstructionInfo& info = DumpRISCVInstruction(instruction.opcode);
	if (info.opcode == RISCVOpcode::None) {
		aasm_log(_LOG_ERROR, "RISC-V instruction encoding is not implemented");
		return false;
	}
	if (!SupportsRISCVXLen(info, session.CurrentBits)) {
		aasm_log(_LOG_ERROR,
			"RISC-V instruction is not available in the current bit mode");
		return false;
	}
	const RISCVOperand* operand = instruction.operands;
	int64_t immediate;

	switch (info.format) {
	case RISCVEncodingFormat::Upper:
		if (!RequireImmediate(operand[1].value, immediate)) return false;
		if (immediate < INT64_C(-0x80000) || immediate > INT64_C(0xfffff)) {
			aasm_log(_LOG_ERROR, "RISC-V upper immediate is outside 20 bits");
			return false;
		}
		encoding = info.fixed_bits | static_cast<uint32_t>(operand[0].reg) << 7 |
			(static_cast<uint32_t>(immediate) & 0xfffff) << 12;
		return true;

	case RISCVEncodingFormat::Jump:
		if (!ResolveTarget(session, operand[1].value, offset, immediate)) return false;
		if ((immediate & 1) || !SignedRange(immediate, 21)) {
			aasm_log(_LOG_ERROR, "RISC-V jump target is outside the J range");
			return false;
		}
		encoding = EncodeJ(info.fixed_bits, operand[0].reg, immediate);
		return true;

	case RISCVEncodingFormat::IndirectJump:
		if (!RequireIImmediate(operand[1], immediate)) return false;
		encoding = EncodeI(info.fixed_bits,
			operand[0].reg, operand[1].reg, immediate);
		return true;

	case RISCVEncodingFormat::Branch:
		if (!ResolveTarget(session, operand[2].value, offset, immediate)) return false;
		if ((immediate & 1) || !SignedRange(immediate, 13)) {
			aasm_log(_LOG_ERROR, "RISC-V branch target is outside the B range");
			return false;
		}
		encoding = EncodeB(info.fixed_bits,
			operand[0].reg, operand[1].reg, immediate);
		return true;

	case RISCVEncodingFormat::Load:
		if (!RequireIImmediate(operand[1], immediate)) return false;
		encoding = EncodeI(info.fixed_bits,
			operand[0].reg, operand[1].reg, immediate);
		return true;

	case RISCVEncodingFormat::Store:
		if (!RequireIImmediate(operand[1], immediate)) return false;
		encoding = EncodeS(info.fixed_bits,
			operand[1].reg, operand[0].reg, immediate);
		return true;

	case RISCVEncodingFormat::Immediate:
	case RISCVEncodingFormat::ImmediateWord:
		if (!RequireIImmediate(operand[2], immediate)) return false;
		encoding = EncodeI(info.fixed_bits,
			operand[0].reg, operand[1].reg, immediate);
		return true;

	case RISCVEncodingFormat::Shift:
	case RISCVEncodingFormat::ShiftWord:
		if (!RequireImmediate(operand[2].value, immediate)) return false;
		if (immediate < 0 || immediate >=
			(info.format == RISCVEncodingFormat::ShiftWord ? 32 :
				session.CurrentBits)) {
			aasm_log(_LOG_ERROR, info.format == RISCVEncodingFormat::ShiftWord ?
				"RISC-V word shift amount is outside 5 bits" :
				"RISC-V shift amount is outside the register width");
			return false;
		}
		encoding = EncodeI(info.fixed_bits,
			operand[0].reg, operand[1].reg, immediate);
		return true;

	case RISCVEncodingFormat::Register:
	case RISCVEncodingFormat::RegisterWord:
		encoding = EncodeR(info.fixed_bits, operand[0].reg,
			operand[1].reg, operand[2].reg);
		return true;

	case RISCVEncodingFormat::System:
		encoding = info.fixed_bits;
		return true;
	}
	aasm_log(_LOG_ERROR, "RISC-V instruction encoding is not implemented");
	return false;
}

}
