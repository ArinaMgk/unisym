// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Instruction Encoder
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/riscv/Instruction-riscv.hpp"
#include "../../../../../inc/cpp/Lango/Archend/riscv/Vector-riscv.hpp"

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

bool CanEncodePcRelativePair(int64_t displacement)
{
	int64_t upper, low;
	SplitSigned12(displacement, upper, low);
	return SignedRange(upper, 20) && SignedRange(low, 12);
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
	Unknown,
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
	if (value.unknown) return TargetResolution::Unknown;
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

enum class ControlFlowForm {
	CompressedDirect,
	Direct,
	BranchJump,
	JumpPair,
	BranchPair
};

struct ControlFlowPlan {
	ControlFlowForm form = ControlFlowForm::Direct;
	TargetResolution resolution = TargetResolution::Resolved;
	int64_t displacement = 0;
	unsigned size = 4;
};

bool IsCompressedJump(RISCVOpcode opcode)
{
	return opcode == RISCVOpcode::CJ || opcode == RISCVOpcode::CJal;
}

bool IsCompressedBranch(RISCVOpcode opcode)
{
	return opcode == RISCVOpcode::CBeqz || opcode == RISCVOpcode::CBnez;
}

uint8_t JumpLinkRegister(const RISCVInstruction& instruction)
{
	if (instruction.opcode == RISCVOpcode::CJal) return 1;
	if (instruction.opcode == RISCVOpcode::CJ) return 0;
	return instruction.operands[0].reg;
}

uint32_t StandardBranchFixed(const RISCVInstruction& instruction,
	const RISCVInstructionInfo& info)
{
	if (instruction.opcode == RISCVOpcode::CBeqz) return 0x00000063;
	if (instruction.opcode == RISCVOpcode::CBnez) return 0x00001063;
	return info.fixed_bits;
}

bool PlanJump(const AssemblySession& session,
	const RISCVInstruction& instruction, int64_t offset,
	ControlFlowPlan& plan)
{
	bool compressed = IsCompressedJump(instruction.opcode);
	const RISCVValue& target = instruction.operands[compressed ? 0 : 1].value;
	plan.resolution = ResolveTarget(session, target, offset, plan.displacement);
	if (plan.resolution == TargetResolution::Error) return false;
	if (plan.resolution == TargetResolution::Unknown) {
		plan.form = compressed ? ControlFlowForm::CompressedDirect :
			ControlFlowForm::Direct;
		plan.size = compressed ? 2 : 4;
		return true;
	}
	if (plan.resolution == TargetResolution::Fixup) {
		if (instruction.has_temporary) {
			plan.form = ControlFlowForm::JumpPair;
			plan.size = 8;
		}
		else {
			plan.form = compressed ? ControlFlowForm::CompressedDirect :
				ControlFlowForm::Direct;
			plan.size = compressed ? 2 : 4;
		}
		return true;
	}
	if (compressed && !(plan.displacement & 1) &&
		SignedRange(plan.displacement, 12)) {
		plan.form = ControlFlowForm::CompressedDirect;
		plan.size = 2;
		return true;
	}
	if (!(plan.displacement & 1) && SignedRange(plan.displacement, 21)) {
		plan.form = ControlFlowForm::Direct;
		plan.size = 4;
		return true;
	}
	uint8_t scratch = instruction.has_temporary ? instruction.temporary_reg :
		JumpLinkRegister(instruction);
	if (!scratch) {
		aasm_log(_LOG_ERROR,
			"RISC-V long jump requires a temporary register");
		return false;
	}
	if (!CanEncodePcRelativePair(plan.displacement)) {
		aasm_log(_LOG_ERROR,
			"RISC-V jump target is outside the PC-relative 32-bit range");
		return false;
	}
	plan.form = ControlFlowForm::JumpPair;
	plan.size = 8;
	return true;
}

bool PlanBranch(const AssemblySession& session,
	const RISCVInstruction& instruction, int64_t offset,
	ControlFlowPlan& plan)
{
	bool compressed = IsCompressedBranch(instruction.opcode);
	const RISCVValue& target = instruction.operands[compressed ? 1 : 2].value;
	plan.resolution = ResolveTarget(session, target, offset, plan.displacement);
	if (plan.resolution == TargetResolution::Error) return false;
	if (plan.resolution == TargetResolution::Unknown) {
		plan.form = compressed ? ControlFlowForm::CompressedDirect :
			ControlFlowForm::Direct;
		plan.size = compressed ? 2 : 4;
		return true;
	}
	if (plan.resolution == TargetResolution::Fixup) {
		if (instruction.has_temporary) {
			plan.form = ControlFlowForm::BranchPair;
			plan.size = 12;
		}
		else {
			plan.form = compressed ? ControlFlowForm::CompressedDirect :
				ControlFlowForm::Direct;
			plan.size = compressed ? 2 : 4;
		}
		return true;
	}
	if (compressed && !(plan.displacement & 1) &&
		SignedRange(plan.displacement, 9)) {
		plan.form = ControlFlowForm::CompressedDirect;
		plan.size = 2;
		return true;
	}
	if (!(plan.displacement & 1) && SignedRange(plan.displacement, 13)) {
		plan.form = ControlFlowForm::Direct;
		plan.size = 4;
		return true;
	}
	int64_t jump_displacement = plan.displacement - 4;
	if (!(jump_displacement & 1) && SignedRange(jump_displacement, 21)) {
		plan.form = ControlFlowForm::BranchJump;
		plan.size = 8;
		return true;
	}
	if (!instruction.has_temporary) {
		aasm_log(_LOG_ERROR,
			"RISC-V long branch requires a temporary register");
		return false;
	}
	if (!CanEncodePcRelativePair(jump_displacement)) {
		aasm_log(_LOG_ERROR,
			"RISC-V branch target is outside the PC-relative 32-bit range");
		return false;
	}
	plan.form = ControlFlowForm::BranchPair;
	plan.size = 12;
	return true;
}

bool Append(RISCVEncodingSequence& sequence, uint64_t encoding,
	RISCVFixupKind fixup = RISCVFixupKind::None,
	const RISCVValue& target = RISCVValue{}, uint8_t size = 4,
	uint64_t pair_offset = 0,
	RISCVFixupPairRole pair_role = RISCVFixupPairRole::None)
{
	if (sequence.count >= numsof(sequence.instructions)) {
		aasm_log(_LOG_PANIC, "RISC-V expansion exceeds its instruction capacity");
		return false;
	}
	RISCVEncodingResult& result = sequence.instructions[sequence.count++];
	result.encoding = encoding;
	result.size = size;
	result.fixup = fixup;
	result.target = target;
	result.pair_offset = pair_offset;
	result.pair_role = pair_role;
	return true;
}

uint16_t EncodeCJ(uint16_t fixed, int64_t immediate)
{
	uint16_t value = static_cast<uint16_t>(immediate);
	return fixed | ((value >> 11) & 1) << 12 | ((value >> 4) & 1) << 11 |
		((value >> 8) & 3) << 9 | ((value >> 10) & 1) << 8 |
		((value >> 6) & 1) << 7 | ((value >> 7) & 1) << 6 |
		((value >> 1) & 7) << 3 | ((value >> 5) & 1) << 2;
}

uint16_t EncodeCB(uint16_t fixed, uint8_t reg, int64_t immediate)
{
	uint16_t value = static_cast<uint16_t>(immediate);
	return fixed | ((value >> 8) & 1) << 12 | ((value >> 3) & 3) << 10 |
		(reg - 8) << 7 | ((value >> 6) & 3) << 5 |
		((value >> 1) & 3) << 3 | ((value >> 5) & 1) << 2;
}

bool AppendPcRelativeJumpPair(RISCVEncodingSequence& sequence,
	int64_t offset, uint8_t scratch, uint8_t link,
	const RISCVValue& target, TargetResolution resolution)
{
	if (resolution == TargetResolution::Resolved) {
		int64_t displacement = target.offset - offset;
		int64_t high, low;
		SplitSigned12(displacement, high, low);
		if (!SignedRange(high, 20)) {
			aasm_log(_LOG_ERROR,
				"RISC-V jump target is outside the PC-relative 32-bit range");
			return false;
		}
		return Append(sequence, EncodeU(0x00000017, scratch, high)) &&
			Append(sequence, EncodeI(0x00000067, link, scratch, low));
	}
	if (resolution != TargetResolution::Fixup) {
		aasm_log(_LOG_ERROR, "RISC-V long jump target is unresolved");
		return false;
	}
	if (!Append(sequence, EncodeU(0x00000017, scratch, 0),
		RISCVFixupKind::PcRelativeHigh, target, 4,
		static_cast<uint64_t>(offset), RISCVFixupPairRole::Head)) return false;
	return Append(sequence, EncodeI(0x00000067, link, scratch, 0),
		RISCVFixupKind::PcRelativeLowI, target, 4,
		static_cast<uint64_t>(offset), RISCVFixupPairRole::Tail);
}

bool EncodeJumpControlFlow(const RISCVInstruction& instruction,
	const RISCVInstructionInfo& info, int64_t offset,
	const ControlFlowPlan& plan, RISCVEncodingSequence& result)
{
	bool compressed = IsCompressedJump(instruction.opcode);
	const RISCVValue& target = instruction.operands[compressed ? 0 : 1].value;
	uint8_t link = JumpLinkRegister(instruction);
	if (plan.form == ControlFlowForm::CompressedDirect) {
		if (plan.resolution == TargetResolution::Fixup)
			return Append(result, static_cast<uint16_t>(info.fixed_bits),
				RISCVFixupKind::CompressedJump, target, 2);
		if (plan.resolution != TargetResolution::Resolved) {
			aasm_log(_LOG_ERROR, "RISC-V jump target is unresolved");
			return false;
		}
		return Append(result,
			EncodeCJ(static_cast<uint16_t>(info.fixed_bits), plan.displacement),
			RISCVFixupKind::None, {}, 2);
	}
	if (plan.form == ControlFlowForm::Direct) {
		uint32_t fixed = compressed ? 0x0000006f : info.fixed_bits;
		if (plan.resolution == TargetResolution::Fixup)
			return Append(result, EncodeJ(fixed, link, 0),
				RISCVFixupKind::Jump, target);
		if (plan.resolution != TargetResolution::Resolved) {
			aasm_log(_LOG_ERROR, "RISC-V jump target is unresolved");
			return false;
		}
		return Append(result, EncodeJ(fixed, link, plan.displacement));
	}
	uint8_t scratch = instruction.has_temporary ? instruction.temporary_reg : link;
	return AppendPcRelativeJumpPair(result, offset, scratch, link,
		target, plan.resolution);
}

bool EncodeBranchControlFlow(const RISCVInstruction& instruction,
	const RISCVInstructionInfo& info, int64_t offset,
	const ControlFlowPlan& plan, RISCVEncodingSequence& result)
{
	bool compressed = IsCompressedBranch(instruction.opcode);
	const RISCVValue& target = instruction.operands[compressed ? 1 : 2].value;
	uint8_t left = instruction.operands[0].reg;
	uint8_t right = compressed ? 0 : instruction.operands[1].reg;
	uint32_t fixed = StandardBranchFixed(instruction, info);
	if (plan.form == ControlFlowForm::CompressedDirect) {
		if (plan.resolution == TargetResolution::Fixup)
			return Append(result,
				static_cast<uint16_t>(info.fixed_bits) | (left - 8) << 7,
				RISCVFixupKind::CompressedBranch, target, 2);
		if (plan.resolution != TargetResolution::Resolved) {
			aasm_log(_LOG_ERROR, "RISC-V branch target is unresolved");
			return false;
		}
		return Append(result,
			EncodeCB(static_cast<uint16_t>(info.fixed_bits),
				left, plan.displacement), RISCVFixupKind::None, {}, 2);
	}
	if (plan.form == ControlFlowForm::Direct) {
		if (plan.resolution == TargetResolution::Fixup)
			return Append(result, EncodeB(fixed, left, right, 0),
				RISCVFixupKind::Branch, target);
		if (plan.resolution != TargetResolution::Resolved) {
			aasm_log(_LOG_ERROR, "RISC-V branch target is unresolved");
			return false;
		}
		return Append(result,
			EncodeB(fixed, left, right, plan.displacement));
	}
	uint32_t inverse = fixed ^ 0x00001000;
	unsigned skip = plan.form == ControlFlowForm::BranchJump ? 8 : 12;
	if (!Append(result, EncodeB(inverse, left, right, skip))) return false;
	if (plan.form == ControlFlowForm::BranchJump) {
		if (plan.resolution != TargetResolution::Resolved) {
			aasm_log(_LOG_ERROR, "RISC-V long branch target is unresolved");
			return false;
		}
		return Append(result,
			EncodeJ(0x0000006f, 0, target.offset - (offset + 4)));
	}
	return AppendPcRelativeJumpPair(result, offset + 4,
		instruction.temporary_reg, 0, target, plan.resolution);
}

bool UsesRoundingMode(const RISCVInstructionInfo& info)
{
	if (info.format == RISCVEncodingFormat::FloatUnary ||
		info.format == RISCVEncodingFormat::FloatConvertToInteger ||
		info.format == RISCVEncodingFormat::FloatConvertFromInteger ||
		info.format == RISCVEncodingFormat::FloatFused) return true;
	if (info.format == RISCVEncodingFormat::FloatConvertFormat)
		return info.opcode == RISCVOpcode::FcvtSD;
	if (info.format != RISCVEncodingFormat::FloatRegister) return false;
	switch (info.opcode) {
	case RISCVOpcode::FaddS: case RISCVOpcode::FsubS:
	case RISCVOpcode::FmulS: case RISCVOpcode::FdivS:
	case RISCVOpcode::FaddD: case RISCVOpcode::FsubD:
	case RISCVOpcode::FmulD: case RISCVOpcode::FdivD:
		return true;
	default:
		return false;
	}
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
		if (!CanEncodePcRelativePair(displacement)) {
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
		RISCVFixupKind::PcRelativeHigh, target, 4,
		static_cast<uint64_t>(offset),
		RISCVFixupPairRole::Head)) return false;
	return Append(sequence, store ?
		EncodeS(second_fixed, address_reg, second_reg, 0) :
		EncodeI(second_fixed, second_reg, address_reg, 0),
		store ? RISCVFixupKind::PcRelativeLowS :
			RISCVFixupKind::PcRelativeLowI, target, 4,
		static_cast<uint64_t>(offset),
		RISCVFixupPairRole::Tail);
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
	const RISCVInstruction& instruction, int64_t offset, unsigned& count)
{
	if (instruction.kind == RISCVInstructionKind::Call ||
		instruction.kind == RISCVInstructionKind::CallPlt) {
		count = 8;
		return true;
	}
	const RISCVInstructionInfo& info = DumpRISCVInstruction(instruction.opcode);
	if (info.format == RISCVEncodingFormat::Jump ||
		IsCompressedJump(instruction.opcode)) {
		ControlFlowPlan plan;
		if (!PlanJump(session, instruction, offset, plan)) return false;
		count = plan.size;
		return true;
	}
	if (info.format == RISCVEncodingFormat::Branch ||
		IsCompressedBranch(instruction.opcode)) {
		ControlFlowPlan plan;
		if (!PlanBranch(session, instruction, offset, plan)) return false;
		count = plan.size;
		return true;
	}
	if (info.format == RISCVEncodingFormat::Compressed) {
		count = 2;
		return true;
	}
	if (instruction.kind == RISCVInstructionKind::MoveExpression) {
		if (!MeasureExpression(session, instruction.operands[1].value,
			false, count)) return false;
		count *= 4;
		return true;
	}
	if ((info.format == RISCVEncodingFormat::Load ||
		info.format == RISCVEncodingFormat::Store) &&
		instruction.operands[1].address_kind == RISCVAddressKind::Expression) {
		if (!MeasureExpression(session, instruction.operands[1].value,
			true, count)) return false;
		count *= 4;
		return true;
	}
	if ((info.format == RISCVEncodingFormat::FloatLoad ||
		info.format == RISCVEncodingFormat::FloatStore) &&
		instruction.operands[1].address_kind == RISCVAddressKind::Expression) {
		if (!MeasureExpression(session, instruction.operands[1].value,
			true, count)) return false;
		count *= 4;
		return true;
	}
	count = 4;
	return true;
}

bool RISCVEncoder::Encode(const AssemblySession& session,
	const RISCVInstruction& instruction, int64_t offset,
	RISCVEncodingSequence& result)
{
	result = RISCVEncodingSequence{};
	if (instruction.kind == RISCVInstructionKind::Call ||
		instruction.kind == RISCVInstructionKind::CallPlt) {
		const RISCVValue& target = instruction.operands[0].value;
		if (target.wrt != NO_SEG || target.unknown) {
			aasm_log(_LOG_ERROR, "RISC-V call target is not relocatable");
			return false;
		}
		if (instruction.kind == RISCVInstructionKind::CallPlt &&
			!target.external && target.section == NO_SEG) {
			aasm_log(_LOG_ERROR,
				"RISC-V CALL_PLT requires a relocatable symbol");
			return false;
		}
		int64_t displacement = 0;
		TargetResolution resolution = ResolveTarget(
			session, target, offset, displacement);
		if (instruction.kind == RISCVInstructionKind::CallPlt ||
			resolution == TargetResolution::Fixup) {
			uint32_t auipc = EncodeU(0x00000017, 1, 0);
			uint32_t jalr = EncodeI(0x00000067, 1, 1, 0);
			uint64_t pair = static_cast<uint64_t>(auipc) |
				static_cast<uint64_t>(jalr) << 32;
			return Append(result, pair,
				instruction.kind == RISCVInstructionKind::CallPlt ?
					RISCVFixupKind::CallPlt : RISCVFixupKind::Call,
				target, 8);
		}
		if (resolution == TargetResolution::Error ||
			(displacement & 1) || !CanEncodePcRelativePair(displacement)) {
			aasm_log(_LOG_ERROR, "RISC-V call target is outside the range");
			return false;
		}
		int64_t high, low;
		SplitSigned12(displacement, high, low);
		return Append(result, EncodeU(0x00000017, 1, high)) &&
			Append(result, EncodeI(0x00000067, 1, 1, low));
	}
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
	if (info.format == RISCVEncodingFormat::Jump ||
		IsCompressedJump(instruction.opcode)) {
		ControlFlowPlan plan;
		if (!PlanJump(session, instruction, offset, plan)) return false;
		return EncodeJumpControlFlow(instruction, info, offset, plan, result);
	}
	if (info.format == RISCVEncodingFormat::Branch ||
		IsCompressedBranch(instruction.opcode)) {
		ControlFlowPlan plan;
		if (!PlanBranch(session, instruction, offset, plan)) return false;
		return EncodeBranchControlFlow(instruction, info, offset, plan, result);
	}
	const RISCVOperand* operand = instruction.operands;
	int64_t immediate;
	uint32_t encoding;
	uint32_t rounding = UsesRoundingMode(info) ?
		static_cast<uint32_t>(instruction.rounding) << 12 : 0;

	switch (info.format) {
	case RISCVEncodingFormat::Upper:
		if (operand[1].relocation != RISCVRelocationKind::None) {
			const RISCVValue& target = operand[1].value;
			if (target.unknown || target.wrt != NO_SEG ||
				(!target.external && target.section == NO_SEG)) {
				aasm_log(_LOG_ERROR,
					"RISC-V relocation requires a relocatable symbol");
				return false;
			}
			RISCVFixupKind fixup;
			if (operand[1].relocation == RISCVRelocationKind::High &&
				instruction.opcode == RISCVOpcode::Lui)
				fixup = RISCVFixupKind::High;
			else if (operand[1].relocation == RISCVRelocationKind::Got &&
				instruction.opcode == RISCVOpcode::Auipc)
				fixup = RISCVFixupKind::GotHigh;
			else if (operand[1].relocation ==
				RISCVRelocationKind::PcRelativeHigh &&
				instruction.opcode == RISCVOpcode::Auipc)
				fixup = RISCVFixupKind::PcRelativeHigh;
			else {
				aasm_log(_LOG_ERROR,
					"RISC-V relocation qualifier is invalid for this instruction");
				return false;
			}
			bool paired = fixup == RISCVFixupKind::GotHigh ||
				fixup == RISCVFixupKind::PcRelativeHigh;
			return Append(result,
				EncodeU(info.fixed_bits, operand[0].reg, 0), fixup, target, 4,
				paired ? static_cast<uint64_t>(offset) : 0,
				paired ? RISCVFixupPairRole::Head : RISCVFixupPairRole::None);
		}
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
		if (operand[1].relocation != RISCVRelocationKind::None) {
			bool pcrel = operand[1].relocation ==
				RISCVRelocationKind::PcRelativeLow;
			if ((!pcrel && operand[1].relocation !=
				RISCVRelocationKind::Low) ||
				operand[1].address_kind != RISCVAddressKind::RegisterRelative ||
				operand[1].value.unknown ||
				operand[1].value.wrt != NO_SEG ||
				(!operand[1].value.external &&
				operand[1].value.section == NO_SEG) ||
				(pcrel && (operand[1].value.external ||
				operand[1].value.section == NO_SEG ||
				operand[1].value.offset < 0))) {
				aasm_log(_LOG_ERROR,
					"RISC-V low relocation is invalid for this load address");
				return false;
			}
			return Append(result,
				EncodeI(info.fixed_bits, operand[0].reg, operand[1].reg, 0),
				pcrel ? RISCVFixupKind::PcRelativeLowI :
					RISCVFixupKind::LowI,
				operand[1].value, 4,
				pcrel ? static_cast<uint64_t>(operand[1].value.offset) : 0,
				pcrel ? RISCVFixupPairRole::Tail : RISCVFixupPairRole::None);
		}
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
		if (operand[1].relocation != RISCVRelocationKind::None) {
			bool pcrel = operand[1].relocation ==
				RISCVRelocationKind::PcRelativeLow;
			if ((!pcrel && operand[1].relocation !=
				RISCVRelocationKind::Low) ||
				operand[1].address_kind != RISCVAddressKind::RegisterRelative ||
				operand[1].value.unknown ||
				operand[1].value.wrt != NO_SEG ||
				(!operand[1].value.external &&
				operand[1].value.section == NO_SEG) ||
				(pcrel && (operand[1].value.external ||
				operand[1].value.section == NO_SEG ||
				operand[1].value.offset < 0))) {
				aasm_log(_LOG_ERROR,
					"RISC-V low relocation is invalid for this store address");
				return false;
			}
			return Append(result,
				EncodeS(info.fixed_bits, operand[1].reg, operand[0].reg, 0),
				pcrel ? RISCVFixupKind::PcRelativeLowS :
					RISCVFixupKind::LowS,
				operand[1].value, 4,
				pcrel ? static_cast<uint64_t>(operand[1].value.offset) : 0,
				pcrel ? RISCVFixupPairRole::Tail : RISCVFixupPairRole::None);
		}
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
		if (operand[2].relocation != RISCVRelocationKind::None) {
			if (operand[2].relocation != RISCVRelocationKind::Low ||
				instruction.opcode != RISCVOpcode::Addi ||
				operand[2].value.unknown ||
				operand[2].value.wrt != NO_SEG ||
				(!operand[2].value.external &&
				operand[2].value.section == NO_SEG)) {
				aasm_log(_LOG_ERROR,
					"RISC-V low relocation requires ADDI and a relocatable symbol");
				return false;
			}
			return Append(result,
				EncodeI(info.fixed_bits, operand[0].reg, operand[1].reg, 0),
				RISCVFixupKind::LowI, operand[2].value);
		}
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

	case RISCVEncodingFormat::CSRRegister:
		return Append(result, info.fixed_bits | operand[0].reg << 7 |
			operand[2].reg << 15 |
			(static_cast<uint32_t>(operand[1].value.offset) & 0xfff) << 20);

	case RISCVEncodingFormat::CSRImmediate:
		if (!RequireImmediate(operand[2].value, immediate)) return false;
		if (immediate < 0 || immediate > 31) {
			aasm_log(_LOG_ERROR,
				"RISC-V CSR immediate is outside the 5-bit range");
			return false;
		}
		return Append(result, info.fixed_bits | operand[0].reg << 7 |
			static_cast<uint32_t>(immediate) << 15 |
			(static_cast<uint32_t>(operand[1].value.offset) & 0xfff) << 20);

	case RISCVEncodingFormat::AtomicLoad:
		encoding = info.fixed_bits | operand[0].reg << 7 |
			operand[1].reg << 15;
		if ((static_cast<uint8_t>(instruction.atomic_ordering) &
			static_cast<uint8_t>(RISCVAtomicOrdering::Acquire)) != 0)
			encoding |= UINT32_C(1) << 26;
		if ((static_cast<uint8_t>(instruction.atomic_ordering) &
			static_cast<uint8_t>(RISCVAtomicOrdering::Release)) != 0)
			encoding |= UINT32_C(1) << 25;
		return Append(result, encoding);

	case RISCVEncodingFormat::Atomic:
		encoding = info.fixed_bits | operand[0].reg << 7 |
			operand[2].reg << 15 | operand[1].reg << 20;
		if ((static_cast<uint8_t>(instruction.atomic_ordering) &
			static_cast<uint8_t>(RISCVAtomicOrdering::Acquire)) != 0)
			encoding |= UINT32_C(1) << 26;
		if ((static_cast<uint8_t>(instruction.atomic_ordering) &
			static_cast<uint8_t>(RISCVAtomicOrdering::Release)) != 0)
			encoding |= UINT32_C(1) << 25;
		return Append(result, encoding);

	case RISCVEncodingFormat::System:
		return Append(result, info.fixed_bits);

	case RISCVEncodingFormat::SystemRegisters:
		return Append(result, info.fixed_bits |
			operand[0].reg << 15 | operand[1].reg << 20);

	case RISCVEncodingFormat::FloatLoad:
	case RISCVEncodingFormat::FloatStore:
		if (operand[1].address_kind == RISCVAddressKind::Expression) {
			uint8_t temporary = operand[2].reg;
			if (!temporary) {
				aasm_log(_LOG_ERROR,
					"RISC-V floating expression-address temporary register cannot be zero");
				return false;
			}
			return AppendExpressionAddress(session, result, offset, temporary,
				info.fixed_bits, operand[0].reg,
				info.format == RISCVEncodingFormat::FloatStore,
				operand[1].value);
		}
		if (!RequireIImmediate(operand[1], immediate)) return false;
		return Append(result, info.format == RISCVEncodingFormat::FloatStore ?
			EncodeS(info.fixed_bits, operand[1].reg, operand[0].reg, immediate) :
			EncodeI(info.fixed_bits, operand[0].reg, operand[1].reg, immediate));

	case RISCVEncodingFormat::FloatRegister:
		return Append(result, info.fixed_bits | operand[0].reg << 7 |
			rounding | operand[1].reg << 15 | operand[2].reg << 20);

	case RISCVEncodingFormat::FloatUnary:
	case RISCVEncodingFormat::FloatConvertToInteger:
	case RISCVEncodingFormat::FloatConvertFromInteger:
	case RISCVEncodingFormat::FloatConvertFormat:
		return Append(result, info.fixed_bits | operand[0].reg << 7 |
			rounding | operand[1].reg << 15);

	case RISCVEncodingFormat::FloatCompare:
		return Append(result, info.fixed_bits | operand[0].reg << 7 |
			operand[1].reg << 15 | operand[2].reg << 20);

	case RISCVEncodingFormat::FloatMoveToInteger:
	case RISCVEncodingFormat::FloatMoveFromInteger:
		return Append(result, info.fixed_bits | operand[0].reg << 7 |
			operand[1].reg << 15);

	case RISCVEncodingFormat::FloatFused:
		return Append(result, info.fixed_bits | operand[0].reg << 7 |
			rounding | operand[1].reg << 15 | operand[2].reg << 20 |
			operand[3].reg << 27);

	case RISCVEncodingFormat::VectorVV:
	case RISCVEncodingFormat::VectorVX:
	case RISCVEncodingFormat::VectorVI:
		encoding = info.fixed_bits | operand[0].reg << 7 |
			operand[1].reg << 20;
		if (info.format == RISCVEncodingFormat::VectorVI) {
			if (!RequireImmediate(operand[2].value, immediate)) return false;
			if (!SignedRange(immediate, 5)) {
				aasm_log(_LOG_ERROR, "RISC-V vector immediate is outside 5 bits");
				return false;
			}
			encoding |= (static_cast<uint32_t>(immediate) & 0x1f) << 15;
		}
		else encoding |= operand[2].reg << 15;
		if (instruction.masked) encoding &= ~(UINT32_C(1) << 25);
		return Append(result, encoding);

	case RISCVEncodingFormat::VectorConfigI:
		if (info.opcode == RISCVOpcode::Vsetivli) {
			if (!RequireImmediate(operand[1].value, immediate)) return false;
			if (immediate < 0 || immediate > 31) {
				aasm_log(_LOG_ERROR, "RISC-V AVL immediate is outside 5 bits");
				return false;
			}
			return Append(result, info.fixed_bits | operand[0].reg << 7 |
				static_cast<uint32_t>(immediate) << 15 |
				EncodeRISCVVectorType(instruction.vector_type) << 20);
		}
		return Append(result, info.fixed_bits | operand[0].reg << 7 |
			operand[1].reg << 15 |
			EncodeRISCVVectorType(instruction.vector_type) << 20);

	case RISCVEncodingFormat::VectorConfigR:
		return Append(result, info.fixed_bits | operand[0].reg << 7 |
			operand[1].reg << 15 | operand[2].reg << 20);

	case RISCVEncodingFormat::VectorLoad:
	case RISCVEncodingFormat::VectorStore:
		if (operand[1].address_kind != RISCVAddressKind::RegisterRelative ||
			operand[1].value.offset || operand[1].value.section != NO_SEG) {
			aasm_log(_LOG_ERROR,
				"RISC-V vector unit-stride address must be a register without offset");
			return false;
		}
		return Append(result, info.fixed_bits | operand[0].reg << 7 |
			operand[1].reg << 15);

	case RISCVEncodingFormat::Compressed:
		break;
	}

	uint16_t compressed = static_cast<uint16_t>(info.fixed_bits);
	if (info.opcode == RISCVOpcode::CNop)
		return Append(result, compressed, RISCVFixupKind::None, {}, 2);
	if (info.opcode == RISCVOpcode::CAddi || info.opcode == RISCVOpcode::CLi ||
		info.opcode == RISCVOpcode::CLui) {
		if (!RequireImmediate(operand[1].value, immediate)) return false;
		if (!SignedRange(immediate, 6) || !operand[0].reg ||
			(info.opcode == RISCVOpcode::CLui && operand[0].reg == 2)) {
			aasm_log(_LOG_ERROR, "RISC-V compressed immediate or register is invalid");
			return false;
		}
		compressed |= operand[0].reg << 7 |
			(static_cast<uint16_t>(immediate) & 0x1f) << 2 |
			((static_cast<uint16_t>(immediate) >> 5) & 1) << 12;
		return Append(result, compressed, RISCVFixupKind::None, {}, 2);
	}
	if (info.opcode == RISCVOpcode::CMv || info.opcode == RISCVOpcode::CAdd) {
		if (!operand[0].reg || !operand[1].reg) {
			aasm_log(_LOG_ERROR, "RISC-V compressed register cannot be zero");
			return false;
		}
		compressed |= operand[0].reg << 7 | operand[1].reg << 2;
		return Append(result, compressed, RISCVFixupKind::None, {}, 2);
	}
	if (info.opcode == RISCVOpcode::CJ || info.opcode == RISCVOpcode::CJal) {
		switch (ResolveTarget(session, operand[0].value, offset, immediate)) {
		case TargetResolution::Fixup:
			return Append(result, compressed, RISCVFixupKind::CompressedJump,
				operand[0].value, 2);
		case TargetResolution::Error: return false;
		case TargetResolution::Resolved: break;
		}
		if ((immediate & 1) || !SignedRange(immediate, 12)) {
			aasm_log(_LOG_ERROR, "RISC-V compressed jump target is outside range");
			return false;
		}
		return Append(result, EncodeCJ(compressed, immediate),
			RISCVFixupKind::None, {}, 2);
	}
	if (info.opcode == RISCVOpcode::CBeqz || info.opcode == RISCVOpcode::CBnez) {
		if (operand[0].reg < 8 || operand[0].reg > 15) {
			aasm_log(_LOG_ERROR, "RISC-V compressed branch requires x8..x15");
			return false;
		}
		switch (ResolveTarget(session, operand[1].value, offset, immediate)) {
		case TargetResolution::Fixup:
			return Append(result, compressed | (operand[0].reg - 8) << 7,
				RISCVFixupKind::CompressedBranch, operand[1].value, 2);
		case TargetResolution::Error: return false;
		case TargetResolution::Resolved: break;
		}
		if ((immediate & 1) || !SignedRange(immediate, 9)) {
			aasm_log(_LOG_ERROR, "RISC-V compressed branch target is outside range");
			return false;
		}
		return Append(result, EncodeCB(compressed, operand[0].reg, immediate),
			RISCVFixupKind::None, {}, 2);
	}
	if (info.opcode == RISCVOpcode::CLw || info.opcode == RISCVOpcode::CSw ||
		info.opcode == RISCVOpcode::CLd || info.opcode == RISCVOpcode::CSd) {
		if (operand[0].reg < 8 || operand[0].reg > 15 ||
			operand[1].reg < 8 || operand[1].reg > 15 ||
			!RequireImmediate(operand[1].value, immediate) || immediate < 0) {
			aasm_log(_LOG_ERROR, "RISC-V compressed memory operand is invalid");
			return false;
		}
		bool wide = info.opcode == RISCVOpcode::CLd || info.opcode == RISCVOpcode::CSd;
		if ((!wide && (immediate & 3 || immediate > 124)) ||
			(wide && (immediate & 7 || immediate > 248))) {
			aasm_log(_LOG_ERROR, "RISC-V compressed memory offset is invalid");
			return false;
		}
		compressed |= (operand[1].reg - 8) << 7;
		if (info.opcode == RISCVOpcode::CLw || info.opcode == RISCVOpcode::CLd)
			compressed |= (operand[0].reg - 8) << 2;
		else compressed |= (operand[0].reg - 8) << 2;
		compressed |= ((immediate >> 3) & 7) << 10;
		if (wide) compressed |= ((immediate >> 6) & 3) << 5;
		else compressed |= ((immediate >> 2) & 1) << 6 |
			((immediate >> 6) & 1) << 5;
		return Append(result, compressed, RISCVFixupKind::None, {}, 2);
	}
	aasm_log(_LOG_ERROR, "RISC-V instruction encoding is not implemented");
	return false;
}

}
