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

void SplitSigned12(int64_t value, int64_t& upper, int64_t& low)
{
	upper = value / INT64_C(0x1000);
	low = value % INT64_C(0x1000);
	if (low > INT64_C(0x7ff)) {
		low -= INT64_C(0x1000);
		upper++;
	}
	else if (low < INT64_C(-0x800)) {
		low += INT64_C(0x1000);
		upper--;
	}
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

enum class TargetResolution {
	Resolved,
	Fixup,
	Error
};

TargetResolution ResolveTarget(const AssemblySession& session,
	const RISCVValue& value, int64_t offset, int64_t& displacement)
{
	if (value.wrt != NO_SEG) {
		aasm_log(_LOG_ERROR, "RISC-V target does not support WRT");
		return TargetResolution::Error;
	}
	if (value.external) return TargetResolution::Fixup;
	if (value.unknown) {
		aasm_log(_LOG_ERROR, "RISC-V target is unresolved");
		return TargetResolution::Error;
	}
	if (value.section != NO_SEG && value.section != session.Location.segment)
		return TargetResolution::Fixup;
	displacement = value.offset - offset;
	return TargetResolution::Resolved;
}

uint32_t EncodeU(uint32_t fixed, uint32_t rd, int64_t immediate)
{
	return fixed | rd << 7 |
		(static_cast<uint32_t>(immediate) & 0xfffff) << 12;
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

bool Append(RISCVEncodingSequence& sequence, uint32_t encoding,
	RISCVFixupKind fixup = RISCVFixupKind::None,
	const RISCVValue& target = RISCVValue{})
{
	if (sequence.count >= numsof(sequence.instructions)) {
		aasm_log(_LOG_PANIC, "RISC-V expansion exceeds its instruction capacity");
		return false;
	}
	RISCVEncodingResult& result = sequence.instructions[sequence.count++];
	result.encoding = encoding;
	result.fixup = fixup;
	result.target = target;
	return true;
}

bool AppendSigned32(RISCVEncodingSequence& sequence, uint8_t reg,
	int64_t value, bool word_mode)
{
	int64_t high, low;
	SplitSigned12(value, high, low);
	if (high && !Append(sequence, EncodeU(0x00000037, reg, high))) return false;
	if (low || !high) {
		uint32_t fixed = high && word_mode ? 0x0000001b : 0x00000013;
		if (!Append(sequence, EncodeI(fixed, reg, high ? reg : 0, low)))
			return false;
	}
	return true;
}

bool AppendRV64Constant(RISCVEncodingSequence& sequence, uint8_t reg,
	int64_t value)
{
	if (SignedRange(value, 32))
		return AppendSigned32(sequence, reg, value, true);
	int64_t upper, low;
	SplitSigned12(value, upper, low);
	if (!AppendRV64Constant(sequence, reg, upper) ||
		!Append(sequence, EncodeI(0x00001013, reg, reg, 12))) return false;
	return !low || Append(sequence, EncodeI(0x00000013, reg, reg, low));
}

bool AppendConstant(RISCVEncodingSequence& sequence, uint8_t reg,
	int bits, int64_t value)
{
	if (bits == 32) {
		if (value < INT64_C(-0x80000000) || value > INT64_C(0xffffffff)) {
			aasm_log(_LOG_ERROR, "RISC-V value is outside the 32-bit range");
			return false;
		}
		int64_t normalized = static_cast<int32_t>(static_cast<uint32_t>(value));
		return AppendSigned32(sequence, reg, normalized, false);
	}
	return AppendRV64Constant(sequence, reg, value);
}

bool AppendPcRelativePair(const AssemblySession& session,
	RISCVEncodingSequence& sequence, int64_t offset, uint8_t address_reg,
	uint32_t second_fixed, uint8_t second_reg, bool store,
	const RISCVValue& target)
{
	if (target.wrt != NO_SEG) {
		aasm_log(_LOG_ERROR, "RISC-V address does not support WRT");
		return false;
	}
	if (target.unknown) {
		aasm_log(_LOG_ERROR, "RISC-V address is unresolved");
		return false;
	}
	bool relocation = target.external ||
		(target.section != NO_SEG && target.section != session.Location.segment);
	if (!relocation) {
		int64_t displacement = target.offset - offset;
		if (!SignedRange(displacement, 32)) {
			aasm_log(_LOG_ERROR,
				"RISC-V PC-relative address is outside the 32-bit range");
			return false;
		}
		int64_t high, low;
		SplitSigned12(displacement, high, low);
		if (!Append(sequence, EncodeU(0x00000017, address_reg, high)))
			return false;
		return Append(sequence, store ?
			EncodeS(second_fixed, address_reg, second_reg, low) :
			EncodeI(second_fixed, second_reg, address_reg, low));
	}
	if (!Append(sequence, EncodeU(0x00000017, address_reg, 0),
		RISCVFixupKind::PcRelativeHigh, target)) return false;
	RISCVValue anchor;
	anchor.section = session.Location.segment;
	anchor.offset = offset;
	return Append(sequence, store ?
		EncodeS(second_fixed, address_reg, second_reg, 0) :
		EncodeI(second_fixed, second_reg, address_reg, 0),
		store ? RISCVFixupKind::PcRelativeLowS :
			RISCVFixupKind::PcRelativeLowI, anchor);
}

bool AppendExpressionAddress(const AssemblySession& session,
	RISCVEncodingSequence& sequence, int64_t offset, uint8_t address_reg,
	uint32_t second_fixed, uint8_t second_reg, bool store,
	const RISCVValue& address)
{
	if (address.wrt != NO_SEG) {
		aasm_log(_LOG_ERROR, "RISC-V address does not support WRT");
		return false;
	}
	if (address.unknown) {
		aasm_log(_LOG_ERROR, "RISC-V address is unresolved");
		return false;
	}
	if (address.section != NO_SEG || address.external)
		return AppendPcRelativePair(session, sequence, offset, address_reg,
			second_fixed, second_reg, store, address);
	if (!AppendConstant(sequence, address_reg, session.CurrentBits,
		address.offset)) return false;
	return Append(sequence, store ?
		EncodeS(second_fixed, address_reg, second_reg, 0) :
		EncodeI(second_fixed, second_reg, address_reg, 0));
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

bool MeasureExpression(const AssemblySession& session, const RISCVValue& value,
	bool memory, unsigned& count)
{
	if (value.unknown) {
		count = (session.CurrentBits == 64 ? 8 : 2) + (memory ? 1 : 0);
		return true;
	}
	if (value.section != NO_SEG || value.external) {
		count = 2;
		return true;
	}
	RISCVEncodingSequence sequence;
	if (!AppendConstant(sequence, 1, session.CurrentBits, value.offset))
		return false;
	count = sequence.count + (memory ? 1 : 0);
	return true;
}

}

bool RISCVEncoder::Measure(const AssemblySession& session,
	const RISCVInstruction& instruction, unsigned& count)
{
	if (instruction.kind == RISCVInstructionKind::MoveExpression)
		return MeasureExpression(session, instruction.operands[1].value,
			false, count);
	const RISCVInstructionInfo& info = DumpRISCVInstruction(instruction.opcode);
	if ((info.format == RISCVEncodingFormat::Load ||
		info.format == RISCVEncodingFormat::Store) &&
		instruction.operands[1].address_kind == RISCVAddressKind::Expression)
		return MeasureExpression(session, instruction.operands[1].value,
			true, count);
	count = 1;
	return true;
}

bool RISCVEncoder::Encode(const AssemblySession& session,
	const RISCVInstruction& instruction, int64_t offset,
	RISCVEncodingSequence& result)
{
	result = RISCVEncodingSequence{};
	if (instruction.kind == RISCVInstructionKind::MoveExpression) {
		const RISCVOperand& destination = instruction.operands[0];
		const RISCVValue& value = instruction.operands[1].value;
		if (value.wrt != NO_SEG) {
			aasm_log(_LOG_ERROR, "RISC-V MOV expression does not support WRT");
			return false;
		}
		if (value.unknown) {
			aasm_log(_LOG_ERROR, "RISC-V MOV expression is unresolved");
			return false;
		}
		if (value.section != NO_SEG || value.external)
			return AppendPcRelativePair(session, result, offset,
				destination.reg, 0x00000013, destination.reg, false, value);
		return AppendConstant(result, destination.reg,
			session.CurrentBits, value.offset);
	}

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
	uint32_t encoding;

	switch (info.format) {
	case RISCVEncodingFormat::Upper:
		if (!RequireImmediate(operand[1].value, immediate)) return false;
		if (immediate < INT64_C(-0x80000) || immediate > INT64_C(0xfffff)) {
			aasm_log(_LOG_ERROR, "RISC-V upper immediate is outside 20 bits");
			return false;
		}
		return Append(result, EncodeU(info.fixed_bits, operand[0].reg, immediate));

	case RISCVEncodingFormat::Jump:
		encoding = EncodeJ(info.fixed_bits, operand[0].reg, 0);
		switch (ResolveTarget(session, operand[1].value, offset, immediate)) {
		case TargetResolution::Fixup:
			return Append(result, encoding, RISCVFixupKind::Jump,
				operand[1].value);
		case TargetResolution::Error:
			return false;
		case TargetResolution::Resolved:
			break;
		}
		if ((immediate & 1) || !SignedRange(immediate, 21)) {
			aasm_log(_LOG_ERROR, "RISC-V jump target is outside the J range");
			return false;
		}
		return Append(result, EncodeJ(info.fixed_bits, operand[0].reg, immediate));

	case RISCVEncodingFormat::IndirectJump:
		if (!RequireIImmediate(operand[1], immediate)) return false;
		return Append(result, EncodeI(info.fixed_bits,
			operand[0].reg, operand[1].reg, immediate));

	case RISCVEncodingFormat::Branch:
		encoding = EncodeB(info.fixed_bits,
			operand[0].reg, operand[1].reg, 0);
		switch (ResolveTarget(session, operand[2].value, offset, immediate)) {
		case TargetResolution::Fixup:
			return Append(result, encoding, RISCVFixupKind::Branch,
				operand[2].value);
		case TargetResolution::Error:
			return false;
		case TargetResolution::Resolved:
			break;
		}
		if ((immediate & 1) || !SignedRange(immediate, 13)) {
			aasm_log(_LOG_ERROR, "RISC-V branch target is outside the B range");
			return false;
		}
		return Append(result, EncodeB(info.fixed_bits,
			operand[0].reg, operand[1].reg, immediate));

	case RISCVEncodingFormat::Load:
		if (operand[1].address_kind == RISCVAddressKind::Expression) {
			if (!operand[0].reg) {
				aasm_log(_LOG_ERROR,
					"RISC-V expression-address load requires a nonzero destination register");
				return false;
			}
			return AppendExpressionAddress(session, result, offset,
				operand[0].reg, info.fixed_bits, operand[0].reg, false,
				operand[1].value);
		}
		if (!RequireIImmediate(operand[1], immediate)) return false;
		return Append(result, EncodeI(info.fixed_bits,
			operand[0].reg, operand[1].reg, immediate));

	case RISCVEncodingFormat::Store:
		if (operand[1].address_kind == RISCVAddressKind::Expression) {
			uint8_t temporary = operand[2].reg;
			if (!temporary) {
				aasm_log(_LOG_ERROR,
					"RISC-V expression-address store temporary register cannot be zero");
				return false;
			}
			if (temporary == operand[0].reg) {
				aasm_log(_LOG_ERROR,
					"RISC-V expression-address store temporary register overlaps the source");
				return false;
			}
			return AppendExpressionAddress(session, result, offset, temporary,
				info.fixed_bits, operand[0].reg, true, operand[1].value);
		}
		if (!RequireIImmediate(operand[1], immediate)) return false;
		return Append(result, EncodeS(info.fixed_bits,
			operand[1].reg, operand[0].reg, immediate));

	case RISCVEncodingFormat::Immediate:
	case RISCVEncodingFormat::ImmediateWord:
		if (!RequireIImmediate(operand[2], immediate)) return false;
		return Append(result, EncodeI(info.fixed_bits,
			operand[0].reg, operand[1].reg, immediate));

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
		return Append(result, EncodeI(info.fixed_bits,
			operand[0].reg, operand[1].reg, immediate));

	case RISCVEncodingFormat::Register:
	case RISCVEncodingFormat::RegisterWord:
		return Append(result, EncodeR(info.fixed_bits, operand[0].reg,
			operand[1].reg, operand[2].reg));

	case RISCVEncodingFormat::System:
		return Append(result, info.fixed_bits);
	}
	aasm_log(_LOG_ERROR, "RISC-V instruction encoding is not implemented");
	return false;
}

}
