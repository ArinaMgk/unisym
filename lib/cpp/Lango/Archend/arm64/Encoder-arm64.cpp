// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM64 Instruction Encoder
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/arm64/Instruction-arm64.hpp"

namespace uni::lango {

namespace {

bool SignedRange(int64_t value, unsigned bits)
{
	int64_t limit = INT64_C(1) << (bits - 1);
	return value >= -limit && value < limit;
}

bool RequireAbsolute(const ARM64Value& value, int64_t& immediate)
{
	if (value.unknown || value.external || value.section != NO_SEG ||
		value.wrt != NO_SEG) {
		aasm_log(_LOG_ERROR, "ARM64 immediate must be an absolute value");
		return false;
	}
	immediate = value.offset;
	return true;
}

bool SameWidth(const ARM64Register& left, const ARM64Register& right)
{
	if (left.width == right.width) return true;
	aasm_log(_LOG_ERROR, "ARM64 register widths must match");
	return false;
}

bool IsGeneralOrZero(const ARM64Register& reg)
{
	return reg.kind != ARM64RegisterKind::StackPointer;
}

bool IsGeneralOrStack(const ARM64Register& reg)
{
	return reg.kind != ARM64RegisterKind::Zero;
}

bool RequireGeneralOrZero(const ARM64Register& reg, const char* context)
{
	if (IsGeneralOrZero(reg)) return true;
	aasm_log(_LOG_ERROR, "ARM64 %s does not accept SP or WSP", context);
	return false;
}

bool RequireGeneralOrStack(const ARM64Register& reg, const char* context)
{
	if (IsGeneralOrStack(reg)) return true;
	aasm_log(_LOG_ERROR, "ARM64 %s does not accept XZR or WZR", context);
	return false;
}

bool RequireBranchRegister(const ARM64Register& reg)
{
	if (reg.width == 64 && reg.kind == ARM64RegisterKind::General) return true;
	aasm_log(_LOG_ERROR,
		"ARM64 branch register must be X0-X30, FP, or LR");
	return false;
}

bool SelectFixed(const ARM64InstructionInfo& info, uint8_t width,
	uint32_t& fixed)
{
	if (width == 32) fixed = info.fixed32;
	else if (width == 64) fixed = info.fixed64;
	else fixed = 0;
	if (fixed) return true;
	aasm_log(_LOG_ERROR, "ARM64 instruction does not support this register width");
	return false;
}

enum class TargetResolution {
	Resolved,
	Deferred,
	Error
};

TargetResolution ResolveTarget(const AssemblySession& session,
	const ARM64Value& value, int64_t offset, int64_t& displacement)
{
	if (value.wrt != NO_SEG) {
		return TargetResolution::Deferred;
	}
	if (value.unknown) return TargetResolution::Deferred;
	if (value.external ||
		(value.section != NO_SEG && value.section != session.Location.segment))
		return TargetResolution::Deferred;
	displacement = value.offset - offset;
	return TargetResolution::Resolved;
}

bool EncodeBranch(const AssemblySession& session, const ARM64Value& target,
	int64_t offset, uint32_t fixed, unsigned immediate_bits,
	ARM64FixupKind fixup, ARM64EncodingResult& result)
{
	int64_t displacement = 0;
	TargetResolution resolution = ResolveTarget(
		session, target, offset, displacement);
	if (resolution == TargetResolution::Error) return false;
	if (resolution == TargetResolution::Deferred) {
		result.encoding = fixed;
		result.fixup = fixup;
		result.target = target;
		return true;
	}
	if ((displacement & 3) || !SignedRange(displacement, immediate_bits + 2)) {
		aasm_log(_LOG_ERROR, "ARM64 branch target is unaligned or out of range");
		return false;
	}
	uint32_t mask = (UINT32_C(1) << immediate_bits) - 1;
	result.encoding = fixed |
		(static_cast<uint32_t>(displacement >> 2) & mask) <<
		(immediate_bits == 19 ? 5 : 0);
	return true;
}

bool EncodeAddSubRegister(const ARM64InstructionInfo& info,
	const ARM64Instruction& instruction, uint32_t& encoding)
{
	const ARM64Register& rd = instruction.operands[0].reg;
	const ARM64Register& rn = instruction.operands[1].reg;
	const ARM64Register& rm = instruction.operands[2].reg;
	if (!SameWidth(rd, rn) || !SameWidth(rd, rm) ||
		!RequireGeneralOrZero(rd, "register ADD/SUB destination") ||
		!RequireGeneralOrZero(rn, "register ADD/SUB operand") ||
		!RequireGeneralOrZero(rm, "register ADD/SUB operand")) return false;
	uint32_t fixed;
	if (!SelectFixed(info, rd.width, fixed)) return false;
	encoding = fixed | rd.index | rn.index << 5 | rm.index << 16;
	return true;
}

bool EncodeAddSubImmediate(const ARM64InstructionInfo& info,
	const ARM64Instruction& instruction, uint32_t& encoding)
{
	const ARM64Register& rd = instruction.operands[0].reg;
	const ARM64Register& rn = instruction.operands[1].reg;
	if (!SameWidth(rd, rn) ||
		!RequireGeneralOrStack(rd, "immediate ADD/SUB destination") ||
		!RequireGeneralOrStack(rn, "immediate ADD/SUB operand")) return false;
	int64_t immediate;
	if (!RequireAbsolute(instruction.operands[2].value, immediate)) return false;
	uint32_t shift = 0;
	if (immediate >= 0 && immediate <= 0xfff) {
		// Unshifted immediate.
	}
	else if (immediate >= 0 && !(immediate & 0xfff) &&
		(immediate >> 12) <= 0xfff) {
		immediate >>= 12;
		shift = UINT32_C(1) << 22;
	}
	else {
		aasm_log(_LOG_ERROR,
			"ARM64 ADD/SUB immediate must fit 12 bits, optionally shifted by 12");
		return false;
	}
	uint32_t fixed;
	if (!SelectFixed(info, rd.width, fixed)) return false;
	encoding = fixed | shift | rd.index | rn.index << 5 |
		static_cast<uint32_t>(immediate) << 10;
	return true;
}

bool EncodeCompareRegister(const ARM64InstructionInfo& info,
	const ARM64Instruction& instruction, uint32_t& encoding)
{
	const ARM64Register& rn = instruction.operands[0].reg;
	const ARM64Register& rm = instruction.operands[1].reg;
	if (!SameWidth(rn, rm) ||
		!RequireGeneralOrZero(rn, "CMP register operand") ||
		!RequireGeneralOrZero(rm, "CMP register operand")) return false;
	uint32_t fixed;
	if (!SelectFixed(info, rn.width, fixed)) return false;
	encoding = fixed | rn.index << 5 | rm.index << 16;
	return true;
}

bool EncodeCompareImmediate(const ARM64InstructionInfo& info,
	const ARM64Instruction& instruction, uint32_t& encoding)
{
	const ARM64Register& rn = instruction.operands[0].reg;
	if (!RequireGeneralOrStack(rn, "CMP immediate operand")) return false;
	int64_t immediate;
	if (!RequireAbsolute(instruction.operands[1].value, immediate)) return false;
	uint32_t shift = 0;
	if (immediate >= 0 && immediate <= 0xfff) {
		// Unshifted immediate.
	}
	else if (immediate >= 0 && !(immediate & 0xfff) &&
		(immediate >> 12) <= 0xfff) {
		immediate >>= 12;
		shift = UINT32_C(1) << 22;
	}
	else {
		aasm_log(_LOG_ERROR,
			"ARM64 CMP immediate must fit 12 bits, optionally shifted by 12");
		return false;
	}
	uint32_t fixed;
	if (!SelectFixed(info, rn.width, fixed)) return false;
	encoding = fixed | shift | rn.index << 5 |
		static_cast<uint32_t>(immediate) << 10;
	return true;
}

bool EncodeLogicalRegister(const ARM64InstructionInfo& info,
	const ARM64Instruction& instruction, uint32_t& encoding)
{
	const ARM64Register& rd = instruction.operands[0].reg;
	const ARM64Register& rn = instruction.operands[1].reg;
	const ARM64Register& rm = instruction.operands[2].reg;
	if (!SameWidth(rd, rn) || !SameWidth(rd, rm) ||
		!RequireGeneralOrZero(rd, "logical destination") ||
		!RequireGeneralOrZero(rn, "logical operand") ||
		!RequireGeneralOrZero(rm, "logical operand")) return false;
	uint32_t fixed;
	if (!SelectFixed(info, rd.width, fixed)) return false;
	encoding = fixed | rd.index | rn.index << 5 | rm.index << 16;
	return true;
}

bool ResolveRegisterAddress(const ARM64Operand& address,
	uint8_t access_size, uint32_t& base, uint32_t& scaled_offset)
{
	if (address.address_kind != ARM64AddressKind::RegisterRelative) {
		aasm_log(_LOG_ERROR,
			"ARM64 minimal load/store requires a register base address");
		return false;
	}
	if (address.reg.width != 64 || address.reg.kind == ARM64RegisterKind::Zero) {
		aasm_log(_LOG_ERROR,
			"ARM64 memory base must be X0-X30, FP, LR, or SP");
		return false;
	}
	int64_t offset;
	if (!RequireAbsolute(address.value, offset)) return false;
	if (offset < 0 || offset % access_size || offset / access_size > 0xfff) {
		aasm_log(_LOG_ERROR,
			"ARM64 load/store offset must be a nonnegative scaled 12-bit value");
		return false;
	}
	base = address.reg.index;
	scaled_offset = static_cast<uint32_t>(offset / access_size);
	return true;
}

bool EncodeLoadStore(const ARM64InstructionInfo& info,
	const ARM64Instruction& instruction, uint32_t& encoding)
{
	const ARM64Register& rt = instruction.operands[0].reg;
	if (rt.kind == ARM64RegisterKind::StackPointer) {
		aasm_log(_LOG_ERROR, "ARM64 load/store data register cannot be SP or WSP");
		return false;
	}
	uint32_t fixed;
	uint8_t access_size;
	if (rt.width == 32) {
		fixed = info.fixed32;
		access_size = info.access32;
	}
	else {
		fixed = info.fixed64;
		access_size = info.access64;
	}
	if (!fixed || !access_size) {
		aasm_log(_LOG_ERROR,
			"ARM64 load/store does not support this data register width");
		return false;
	}
	uint32_t base, scaled_offset;
	if (!ResolveRegisterAddress(
		instruction.operands[1], access_size, base, scaled_offset)) return false;
	encoding = fixed | rt.index | base << 5 | scaled_offset << 10;
	return true;
}

bool PlanMoveImmediate(const ARM64Register& destination, int64_t value,
	ARM64EncodingSequence& result)
{
	if (destination.kind != ARM64RegisterKind::General) {
		aasm_log(_LOG_ERROR,
			"ARM64 immediate MOV destination must be a general register");
		return false;
	}
	unsigned halfword_count = destination.width == 32 ? 2 : 4;
	if (destination.width == 32 &&
		(value < INT64_C(-0x80000000) || value > INT64_C(0xffffffff))) {
		aasm_log(_LOG_ERROR, "ARM64 W-register MOV immediate is out of range");
		return false;
	}
	uint64_t bits = static_cast<uint64_t>(value);
	if (destination.width == 32) bits &= UINT64_C(0xffffffff);
	uint16_t halves[4];
	unsigned zero_count = 0;
	unsigned ones_count = 0;
	for (unsigned index = 0; index < halfword_count; index++) {
		halves[index] = static_cast<uint16_t>(bits >> (index * 16));
		if (halves[index] != 0) zero_count++;
		if (halves[index] != 0xffff) ones_count++;
	}
	bool use_movn = ones_count < zero_count;
	unsigned first = 0;
	bool found_first = false;
	if (use_movn) {
		for (unsigned index = 0; index < halfword_count; index++)
			if (halves[index] != 0xffff) {
				first = index;
				found_first = true;
				break;
			}
	}
	else {
		for (unsigned index = 0; index < halfword_count; index++)
			if (halves[index] != 0) {
				first = index;
				found_first = true;
				break;
			}
	}
	if (!found_first) first = 0;
	uint32_t first_base;
	uint32_t keep_base;
	if (destination.width == 32) {
		first_base = use_movn ? UINT32_C(0x12800000) : UINT32_C(0x52800000);
		keep_base = UINT32_C(0x72800000);
	}
	else {
		first_base = use_movn ? UINT32_C(0x92800000) : UINT32_C(0xd2800000);
		keep_base = UINT32_C(0xf2800000);
	}
	uint16_t first_value = use_movn ?
		static_cast<uint16_t>(~halves[first]) : halves[first];
	result.instructions[result.count++].encoding = first_base |
		destination.index | static_cast<uint32_t>(first_value) << 5 |
		first << 21;
	for (unsigned index = 0; index < halfword_count; index++) {
		if (index == first) continue;
		if ((!use_movn && halves[index] == 0) ||
			(use_movn && halves[index] == 0xffff)) continue;
		result.instructions[result.count++].encoding = keep_base |
			destination.index | static_cast<uint32_t>(halves[index]) << 5 |
			index << 21;
	}
	return true;
}

bool PlanMoveAddress(const ARM64Register& destination,
	const ARM64Value& target, int64_t offset, ARM64EncodingSequence& result)
{
	if (destination.width != 64 ||
		destination.kind != ARM64RegisterKind::General) {
		aasm_log(_LOG_ERROR,
			"ARM64 label address MOV requires an X general register");
		return false;
	}
	ARM64EncodingResult& page = result.instructions[result.count++];
	page.encoding = UINT32_C(0x90000000) | destination.index;
	page.fixup = ARM64FixupKind::PageRelative;
	page.target = target;
	page.pair_offset = static_cast<uint64_t>(offset);
	page.pair_role = ARM64FixupPairRole::Head;
	ARM64EncodingResult& low = result.instructions[result.count++];
	low.encoding = UINT32_C(0x91000000) | destination.index |
		destination.index << 5;
	low.fixup = ARM64FixupKind::PageOffset;
	low.target = target;
	low.pair_offset = static_cast<uint64_t>(offset);
	low.pair_role = ARM64FixupPairRole::Tail;
	return true;
}

bool SelectMoveMemoryOpcode(bool load, uint8_t width,
	AssemblySignedness signedness, uint8_t register_width,
	ARM64Opcode& opcode)
{
	if (!load) {
		if (width == 1) opcode = ARM64Opcode::Strb;
		else if (width == 2) opcode = ARM64Opcode::Strh;
		else if (width == 4) opcode = ARM64Opcode::Str;
		else if (width == 8) opcode = ARM64Opcode::Str;
		else return false;
		return (width == 8) == (register_width == 64);
	}
	bool signed_load = signedness == AssemblySignedness::Signed;
	if (width == 1)
		opcode = signed_load ? ARM64Opcode::Ldrsb : ARM64Opcode::Ldrb;
	else if (width == 2)
		opcode = signed_load ? ARM64Opcode::Ldrsh : ARM64Opcode::Ldrh;
	else if (width == 4)
		opcode = signed_load && register_width == 64 ?
			ARM64Opcode::Ldrsw : ARM64Opcode::Ldr;
	else if (width == 8) opcode = ARM64Opcode::Ldr;
	else return false;
	if (width == 8) return register_width == 64;
	return register_width == 32 || register_width == 64;
}

bool EncodeMoveMemory(const ARM64Instruction& instruction, bool load,
	uint32_t& encoding)
{
	const ARM64Operand& data = instruction.operands[load ? 0 : 1];
	const ARM64Operand& address = instruction.operands[load ? 1 : 0];
	ARM64Opcode opcode;
	if (!SelectMoveMemoryOpcode(load, address.width,
		instruction.signedness, data.reg.width, opcode)) {
		aasm_log(_LOG_ERROR,
			"ARM64 MOV memory width does not match the data register");
		return false;
	}
	ARM64Instruction native;
	native.opcode = opcode;
	native.operands[0] = data;
	if (load && instruction.signedness != AssemblySignedness::Signed &&
		address.width < 8)
		native.operands[0].reg.width = 32;
	native.operands[1] = address;
	native.operand_count = 2;
	return EncodeLoadStore(DumpARM64Instruction(opcode), native, encoding);
}

bool EncodeOne(const AssemblySession& session,
	const ARM64Instruction& instruction, int64_t offset,
	ARM64EncodingSequence& result)
{
	if (instruction.kind == ARM64InstructionKind::ConditionalBranch) {
		ARM64EncodingResult encoded;
		if (!EncodeBranch(session, instruction.operands[0].value, offset,
			UINT32_C(0x54000000) |
			static_cast<uint32_t>(instruction.condition), 19,
			ARM64FixupKind::ConditionalBranch, encoded)) return false;
		result.instructions[result.count++] = encoded;
		return true;
	}
	if (instruction.kind == ARM64InstructionKind::MoveRegister) {
		const ARM64Register& rd = instruction.operands[0].reg;
		const ARM64Register& rm = instruction.operands[1].reg;
		if (!SameWidth(rd, rm) ||
			!RequireGeneralOrZero(rd, "MOV destination") ||
			!RequireGeneralOrZero(rm, "MOV source")) return false;
		uint32_t fixed = rd.width == 32 ?
			UINT32_C(0x2a0003e0) : UINT32_C(0xaa0003e0);
		result.instructions[result.count++].encoding =
			fixed | rd.index | rm.index << 16;
		return true;
	}
	if (instruction.kind == ARM64InstructionKind::MoveImmediate) {
		const ARM64Value& value = instruction.operands[1].value;
		if (value.unknown || value.external || value.section != NO_SEG ||
			value.wrt != NO_SEG)
			return PlanMoveAddress(
				instruction.operands[0].reg, value, offset, result);
		int64_t immediate;
		if (!RequireAbsolute(value, immediate)) return false;
		return PlanMoveImmediate(instruction.operands[0].reg, immediate, result);
	}
	if (instruction.kind == ARM64InstructionKind::MoveLoad ||
		instruction.kind == ARM64InstructionKind::MoveStore) {
		uint32_t encoding;
		if (!EncodeMoveMemory(instruction,
			instruction.kind == ARM64InstructionKind::MoveLoad, encoding)) return false;
		result.instructions[result.count++].encoding = encoding;
		return true;
	}

	const ARM64InstructionInfo& info = DumpARM64Instruction(instruction.opcode);
	ARM64EncodingResult encoded;
	switch (info.format) {
	case ARM64EncodingFormat::Fixed:
		encoded.encoding = info.fixed64;
		break;
	case ARM64EncodingFormat::Branch:
		if (!EncodeBranch(session, instruction.operands[0].value,
			offset, info.fixed64, 26,
			instruction.opcode == ARM64Opcode::Bl ?
				ARM64FixupKind::Call : ARM64FixupKind::Branch,
			encoded)) return false;
		break;
	case ARM64EncodingFormat::RegisterBranch:
		if (!RequireBranchRegister(instruction.operands[0].reg)) return false;
		encoded.encoding = info.fixed64 |
			instruction.operands[0].reg.index << 5;
		break;
	case ARM64EncodingFormat::AddSubRegister:
		if (!EncodeAddSubRegister(
			info, instruction, encoded.encoding)) return false;
		break;
	case ARM64EncodingFormat::AddSubImmediate:
		if (!EncodeAddSubImmediate(
			info, instruction, encoded.encoding)) return false;
		break;
	case ARM64EncodingFormat::CompareRegister:
		if (!EncodeCompareRegister(
			info, instruction, encoded.encoding)) return false;
		break;
	case ARM64EncodingFormat::CompareImmediate:
		if (!EncodeCompareImmediate(
			info, instruction, encoded.encoding)) return false;
		break;
	case ARM64EncodingFormat::LogicalRegister:
		if (!EncodeLogicalRegister(
			info, instruction, encoded.encoding)) return false;
		break;
	case ARM64EncodingFormat::LoadStore:
		if (!EncodeLoadStore(
			info, instruction, encoded.encoding)) return false;
		break;
	case ARM64EncodingFormat::ConditionalBranch:
		aasm_log(_LOG_PANIC, "invalid ARM64 instruction-table format");
		return false;
	}
	result.instructions[result.count++] = encoded;
	return true;
}

}

bool ARM64Encoder::Measure(const AssemblySession& session,
	const ARM64Instruction& instruction, int64_t offset, unsigned& size)
{
	ARM64EncodingSequence sequence;
	if (!EncodeOne(session, instruction, offset, sequence)) return false;
	size = sequence.count * 4;
	return true;
}

bool ARM64Encoder::Encode(const AssemblySession& session,
	const ARM64Instruction& instruction,
	int64_t offset, ARM64EncodingSequence& result)
{
	return EncodeOne(session, instruction, offset, result);
}

}
