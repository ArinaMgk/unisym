// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM32 Instruction Encoder
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/arm32/Instruction-arm32.hpp"

namespace uni::lango {

namespace {

uint32_t ConditionBits(ARM32Condition condition)
{
	return static_cast<uint32_t>(condition) << 28;
}

bool SignedRange(int64_t value, unsigned bits)
{
	int64_t limit = INT64_C(1) << (bits - 1);
	return value >= -limit && value < limit;
}

bool RequireAbsolute(const ARM32Value& value, int64_t& immediate)
{
	if (value.unknown || value.external || value.section != NO_SEG ||
		value.wrt != NO_SEG) {
		aasm_log(_LOG_ERROR, "ARM32 immediate must be an absolute value");
		return false;
	}
	immediate = value.offset;
	return true;
}

bool RequireGeneral(const ARM32Register& reg, const char* context)
{
	if (reg.index < 15) return true;
	aasm_log(_LOG_ERROR, "ARM32 %s does not accept PC", context);
	return false;
}

uint32_t RotateRight(uint32_t value, unsigned amount)
{
	amount &= 31;
	if (!amount) return value;
	return value >> amount | value << (32 - amount);
}

uint32_t RotateLeft(uint32_t value, unsigned amount)
{
	amount &= 31;
	if (!amount) return value;
	return value << amount | value >> (32 - amount);
}

bool EncodeModifiedImmediate(uint32_t value, uint32_t& encoded)
{
	for (unsigned rotate = 0; rotate < 16; rotate++) {
		unsigned amount = rotate * 2;
		uint32_t candidate = RotateLeft(value, amount);
		if ((candidate & ~UINT32_C(0xff)) == 0 &&
			RotateRight(candidate, amount) == value) {
			encoded = rotate << 8 | candidate;
			return true;
		}
	}
	return false;
}

bool EncodeShift(const ARM32Shift& shift, uint32_t& encoded)
{
	if (shift.kind == ARM32ShiftKind::None) {
		encoded = 0;
		return true;
	}
	int64_t amount;
	if (!RequireAbsolute(shift.amount, amount)) return false;
	uint32_t type = 0;
	uint32_t field = 0;
	switch (shift.kind) {
	case ARM32ShiftKind::LSL:
		type = 0;
		if (amount < 0 || amount > 31) {
			aasm_log(_LOG_ERROR, "ARM32 LSL amount must be between 0 and 31");
			return false;
		}
		field = static_cast<uint32_t>(amount);
		break;
	case ARM32ShiftKind::LSR:
		type = 1;
		if (amount < 1 || amount > 32) {
			aasm_log(_LOG_ERROR, "ARM32 LSR amount must be between 1 and 32");
			return false;
		}
		field = amount == 32 ? 0 : static_cast<uint32_t>(amount);
		break;
	case ARM32ShiftKind::ASR:
		type = 2;
		if (amount < 1 || amount > 32) {
			aasm_log(_LOG_ERROR, "ARM32 ASR amount must be between 1 and 32");
			return false;
		}
		field = amount == 32 ? 0 : static_cast<uint32_t>(amount);
		break;
	case ARM32ShiftKind::ROR:
		type = 3;
		if (amount < 1 || amount > 31) {
			aasm_log(_LOG_ERROR, "ARM32 ROR amount must be between 1 and 31");
			return false;
		}
		field = static_cast<uint32_t>(amount);
		break;
	case ARM32ShiftKind::None:
		return false;
	}
	encoded = field << 7 | type << 5;
	return true;
}

bool EncodeOperand2(const ARM32Operand& operand, uint32_t& encoded)
{
	if (operand.kind == ARM32OperandKind::Register) {
		if (!RequireGeneral(operand.reg, "data-processing operand")) return false;
		uint32_t shift;
		if (!EncodeShift(operand.shift, shift)) return false;
		encoded = operand.reg.index | shift;
		return true;
	}
	if (operand.kind != ARM32OperandKind::Immediate) return false;
	int64_t immediate;
	if (!RequireAbsolute(operand.value, immediate)) return false;
	if (immediate < INT64_C(-0x80000000) ||
		immediate > INT64_C(0xffffffff)) {
		aasm_log(_LOG_ERROR,
			"ARM32 data-processing immediate is outside the 32-bit range");
		return false;
	}
	uint32_t modified;
	if (!EncodeModifiedImmediate(static_cast<uint32_t>(immediate), modified)) {
		aasm_log(_LOG_ERROR,
			"ARM32 data-processing immediate cannot use modified-immediate encoding");
		return false;
	}
	encoded = UINT32_C(1) << 25 | modified;
	return true;
}

bool EncodeDataProcessing(const ARM32InstructionInfo& info,
	const ARM32Instruction& instruction, uint32_t& encoding)
{
	const ARM32Register& rd = instruction.operands[0].reg;
	const ARM32Register& rn = instruction.operands[1].reg;
	if (!RequireGeneral(rd, "data-processing destination") ||
		!RequireGeneral(rn, "data-processing operand")) return false;
	uint32_t operand2;
	if (!EncodeOperand2(instruction.operands[2], operand2)) return false;
	encoding = ConditionBits(instruction.condition) | info.fixed |
		(instruction.set_flags ? UINT32_C(1) << 20 : 0) |
		rn.index << 16 | rd.index << 12 | operand2;
	return true;
}

bool EncodeCompare(const ARM32InstructionInfo& info,
	const ARM32Instruction& instruction, uint32_t& encoding)
{
	const ARM32Register& rn = instruction.operands[0].reg;
	if (!RequireGeneral(rn, "CMP operand")) return false;
	uint32_t operand2;
	if (!EncodeOperand2(instruction.operands[1], operand2)) return false;
	encoding = ConditionBits(instruction.condition) | info.fixed |
		rn.index << 16 | operand2;
	return true;
}

bool EncodeMultiply(const ARM32InstructionInfo& info,
	const ARM32Instruction& instruction, uint32_t& encoding)
{
	const ARM32Register& rd = instruction.operands[0].reg;
	const ARM32Register& rm = instruction.operands[1].reg;
	const ARM32Register& rs = instruction.operands[2].reg;
	if (!RequireGeneral(rd, "MUL destination") ||
		!RequireGeneral(rm, "MUL operand") ||
		!RequireGeneral(rs, "MUL operand")) return false;
	encoding = ConditionBits(instruction.condition) | info.fixed |
		(instruction.set_flags ? UINT32_C(1) << 20 : 0) |
		rd.index << 16 | rs.index << 8 | rm.index;
	return true;
}

bool ResolveRegisterAddress(const ARM32Operand& address,
	unsigned maximum, uint32_t& base, uint32_t& magnitude, bool& positive)
{
	if (address.address_kind != ARM32AddressKind::RegisterRelative) {
		aasm_log(_LOG_ERROR,
			"ARM32 minimal load/store requires a register base address");
		return false;
	}
	if (!RequireGeneral(address.reg, "memory base")) return false;
	int64_t offset;
	if (!RequireAbsolute(address.value, offset)) return false;
	if (offset < -static_cast<int64_t>(maximum) ||
		offset > static_cast<int64_t>(maximum)) {
		aasm_log(_LOG_ERROR, "ARM32 load/store offset is out of range");
		return false;
	}
	positive = offset >= 0;
	magnitude = static_cast<uint32_t>(positive ? offset : -offset);
	base = address.reg.index;
	return true;
}

bool EncodeLoadStore(const ARM32InstructionInfo& info,
	const ARM32Instruction& instruction, uint32_t& encoding)
{
	const ARM32Register& rt = instruction.operands[0].reg;
	if (!RequireGeneral(rt, "load/store data register")) return false;
	uint32_t base, magnitude;
	bool positive;
	unsigned maximum = info.format == ARM32EncodingFormat::LoadStore ?
		0xfff : 0xff;
	if (!ResolveRegisterAddress(instruction.operands[1], maximum,
		base, magnitude, positive)) return false;
	uint32_t fixed = positive ? info.fixed :
		(info.fixed & ~UINT32_C(0x00800000));
	uint32_t immediate = info.format == ARM32EncodingFormat::LoadStore ?
		magnitude : ((magnitude & 0xf0) << 4 | (magnitude & 0x0f));
	encoding = ConditionBits(instruction.condition) | fixed |
		base << 16 | rt.index << 12 | immediate;
	return true;
}

enum class TargetResolution {
	Resolved,
	Deferred,
	Error
};

TargetResolution ResolveTarget(const AssemblySession& session,
	const ARM32Value& value, int64_t offset, int64_t& displacement)
{
	if (value.wrt != NO_SEG) {
		return TargetResolution::Deferred;
	}
	if (value.unknown) return TargetResolution::Deferred;
	if (value.external ||
		(value.section != NO_SEG && value.section != session.Location.segment))
		return TargetResolution::Deferred;
	displacement = value.offset - (offset + 8);
	return TargetResolution::Resolved;
}

bool EncodeBranch(const AssemblySession& session,
	const ARM32Instruction& instruction, int64_t offset,
	uint32_t fixed, ARM32FixupKind fixup, ARM32EncodingResult& result)
{
	int64_t displacement = 0;
	TargetResolution resolution = ResolveTarget(
		session, instruction.operands[0].value, offset, displacement);
	if (resolution == TargetResolution::Error) return false;
	if (resolution == TargetResolution::Deferred) {
		result.encoding = ConditionBits(instruction.condition) | fixed;
		result.fixup = fixup;
		result.target = instruction.operands[0].value;
		return true;
	}
	if ((displacement & 3) || !SignedRange(displacement, 26)) {
		aasm_log(_LOG_ERROR, "ARM32 branch target is unaligned or out of range");
		return false;
	}
	result.encoding = ConditionBits(instruction.condition) | fixed |
		(static_cast<uint32_t>(displacement >> 2) & UINT32_C(0x00ffffff));
	return true;
}

bool EncodeMoveOperand(const ARM32Instruction& instruction,
	int64_t offset, ARM32EncodingSequence& result)
{
	const ARM32Register& rd = instruction.operands[0].reg;
	if (!RequireGeneral(rd, "MOV destination")) return false;
	const ARM32Operand& source = instruction.operands[1];
	uint32_t condition = ConditionBits(instruction.condition);
	if (source.kind == ARM32OperandKind::Register) {
		uint32_t operand2;
		if (!EncodeOperand2(source, operand2)) return false;
		result.instructions[result.count++].encoding = condition |
			UINT32_C(0x01a00000) |
			(instruction.set_flags ? UINT32_C(1) << 20 : 0) |
			rd.index << 12 | operand2;
		return true;
	}
	if (source.kind != ARM32OperandKind::Immediate) return false;
	if (source.value.unknown || source.value.external ||
		source.value.section != NO_SEG || source.value.wrt != NO_SEG) {
		ARM32EncodingResult& low = result.instructions[result.count++];
		low.encoding = condition | UINT32_C(0x03000000) | rd.index << 12;
		low.fixup = ARM32FixupKind::AddressLow;
		low.target = source.value;
		low.pair_offset = static_cast<uint64_t>(offset);
		low.pair_role = ARM32FixupPairRole::Head;
		ARM32EncodingResult& high = result.instructions[result.count++];
		high.encoding = condition | UINT32_C(0x03400000) | rd.index << 12;
		high.fixup = ARM32FixupKind::AddressHigh;
		high.target = source.value;
		high.pair_offset = static_cast<uint64_t>(offset);
		high.pair_role = ARM32FixupPairRole::Tail;
		return true;
	}
	int64_t immediate;
	if (!RequireAbsolute(source.value, immediate)) return false;
	if (immediate < INT64_C(-0x80000000) ||
		immediate > INT64_C(0xffffffff)) {
		aasm_log(_LOG_ERROR, "ARM32 MOV immediate is out of range");
		return false;
	}
	uint32_t value = static_cast<uint32_t>(immediate);
	uint32_t modified;
	if (EncodeModifiedImmediate(value, modified)) {
		result.instructions[result.count++].encoding = condition |
			UINT32_C(0x03a00000) | rd.index << 12 | modified;
		return true;
	}
	if (EncodeModifiedImmediate(~value, modified)) {
		result.instructions[result.count++].encoding = condition |
			UINT32_C(0x03e00000) | rd.index << 12 | modified;
		return true;
	}
	uint32_t low = value & 0xffff;
	uint32_t high = value >> 16;
	result.instructions[result.count++].encoding = condition |
		UINT32_C(0x03000000) | (low >> 12) << 16 |
		rd.index << 12 | (low & 0xfff);
	if (high)
		result.instructions[result.count++].encoding = condition |
			UINT32_C(0x03400000) | (high >> 12) << 16 |
			rd.index << 12 | (high & 0xfff);
	return true;
}

bool SelectMoveMemoryOpcode(bool load, uint8_t width,
	AssemblySignedness signedness, ARM32Opcode& opcode)
{
	if (!load) {
		if (width == 1) opcode = ARM32Opcode::Strb;
		else if (width == 2) opcode = ARM32Opcode::Strh;
		else if (width == 4) opcode = ARM32Opcode::Str;
		else return false;
		return true;
	}
	bool signed_load = signedness == AssemblySignedness::Signed;
	if (width == 1)
		opcode = signed_load ? ARM32Opcode::Ldrsb : ARM32Opcode::Ldrb;
	else if (width == 2)
		opcode = signed_load ? ARM32Opcode::Ldrsh : ARM32Opcode::Ldrh;
	else if (width == 4) opcode = ARM32Opcode::Ldr;
	else return false;
	return true;
}

bool EncodeMoveMemory(const ARM32Instruction& instruction, bool load,
	uint32_t& encoding)
{
	const ARM32Operand& data = instruction.operands[load ? 0 : 1];
	const ARM32Operand& address = instruction.operands[load ? 1 : 0];
	ARM32Opcode opcode;
	if (!SelectMoveMemoryOpcode(
		load, address.width, instruction.signedness, opcode)) {
		aasm_log(_LOG_ERROR, "ARM32 MOV memory width is invalid");
		return false;
	}
	ARM32Instruction native;
	native.opcode = opcode;
	native.condition = instruction.condition;
	native.operands[0] = data;
	native.operands[1] = address;
	native.operand_count = 2;
	return EncodeLoadStore(DumpARM32Instruction(opcode), native, encoding);
}

bool EncodeOne(const AssemblySession& session,
	const ARM32Instruction& instruction, int64_t offset,
	ARM32EncodingSequence& result)
{
	if (instruction.kind == ARM32InstructionKind::MoveOperand)
		return EncodeMoveOperand(instruction, offset, result);
	if (instruction.kind == ARM32InstructionKind::MoveLoad ||
		instruction.kind == ARM32InstructionKind::MoveStore) {
		uint32_t encoding;
		if (!EncodeMoveMemory(instruction,
			instruction.kind == ARM32InstructionKind::MoveLoad, encoding)) return false;
		result.instructions[result.count++].encoding = encoding;
		return true;
	}

	const ARM32InstructionInfo& info = DumpARM32Instruction(instruction.opcode);
	ARM32EncodingResult encoded;
	switch (info.format) {
	case ARM32EncodingFormat::Fixed:
		encoded.encoding = ConditionBits(instruction.condition) | info.fixed;
		break;
	case ARM32EncodingFormat::Branch:
		if (!EncodeBranch(session, instruction, offset,
			info.fixed, instruction.opcode == ARM32Opcode::Bl &&
				instruction.condition == ARM32Condition::AL ?
				ARM32FixupKind::Call : ARM32FixupKind::Branch,
			encoded)) return false;
		break;
	case ARM32EncodingFormat::RegisterBranch:
		if (!RequireGeneral(
			instruction.operands[0].reg, "branch register")) return false;
		encoded.encoding = ConditionBits(instruction.condition) | info.fixed |
			instruction.operands[0].reg.index;
		break;
	case ARM32EncodingFormat::DataProcessing:
		if (!EncodeDataProcessing(
			info, instruction, encoded.encoding)) return false;
		break;
	case ARM32EncodingFormat::Compare:
		if (!EncodeCompare(info, instruction, encoded.encoding)) return false;
		break;
	case ARM32EncodingFormat::Multiply:
		if (!EncodeMultiply(info, instruction, encoded.encoding)) return false;
		break;
	case ARM32EncodingFormat::LoadStore:
	case ARM32EncodingFormat::LoadStoreExtra:
		if (!EncodeLoadStore(
			info, instruction, encoded.encoding)) return false;
		break;
	}
	result.instructions[result.count++] = encoded;
	return true;
}

bool EncodeT32(const AssemblySession& session,
	const ARM32Instruction& instruction, int64_t offset,
	ARM32EncodingSequence& result)
{
	ARM32EncodingResult encoded;
	encoded.size = 2;
	uint32_t word = 0;
	const ARM32InstructionInfo& info = DumpARM32Instruction(instruction.opcode);
	const ARM32Operand* a = instruction.operands;
	auto low_register = [](const ARM32Operand& operand) {
		return operand.kind == ARM32OperandKind::Register &&
			operand.reg.index < 8;
	};
	auto absolute = [](const ARM32Value& value, int64_t& number) {
		return !value.unknown && !value.external && value.section == NO_SEG &&
			value.wrt == NO_SEG && (number = value.offset, true);
	};
	if (instruction.kind == ARM32InstructionKind::MoveOperand) {
		if (a[1].kind == ARM32OperandKind::Register) {
			if (a[1].reg.index > 15 || a[1].shift.kind != ARM32ShiftKind::None)
				goto unsupported;
			if (instruction.set_flags) {
				if (!low_register(a[0]) || a[1].reg.index > 7)
					goto unsupported;
				word = static_cast<uint32_t>(a[1].reg.index) << 3 |
					a[0].reg.index;
			}
			else {
				if (a[0].reg.index == 13) {
					if (a[1].reg.index > 12) goto unsupported;
				}
				else if (!low_register(a[0]))
					goto unsupported;
				word = UINT32_C(0x4600) |
					(static_cast<uint32_t>(a[1].reg.index) << 3) |
					(a[0].reg.index & 7) |
					((a[0].reg.index & 8) << 4);
			}
		}
		else {
			int64_t immediate;
			if (!low_register(a[0]) || !instruction.set_flags ||
				!absolute(a[1].value, immediate) || immediate < 0 ||
				immediate > 255) goto unsupported;
			word = UINT32_C(0x2000) | a[0].reg.index << 8 |
				static_cast<uint32_t>(immediate);
		}
	}
	else if (info.format == ARM32EncodingFormat::Fixed &&
		instruction.opcode == ARM32Opcode::Nop) {
		word = UINT32_C(0xbf00);
	}
	else if (info.format == ARM32EncodingFormat::DataProcessing &&
		(instruction.opcode == ARM32Opcode::Add ||
		 instruction.opcode == ARM32Opcode::Sub)) {
		if (!instruction.set_flags || !low_register(a[0]) ||
			!low_register(a[1])) goto unsupported;
		if (a[2].kind == ARM32OperandKind::Register && low_register(a[2])) {
			word = (instruction.opcode == ARM32Opcode::Add ?
				UINT32_C(0x1800) : UINT32_C(0x1a00)) |
				a[2].reg.index << 6 | a[1].reg.index << 3 |
				a[0].reg.index;
		}
		else if (a[2].kind == ARM32OperandKind::Immediate) {
			int64_t immediate;
			if (!absolute(a[2].value, immediate) || immediate < 0 ||
				immediate > 255 || a[0].reg.index != a[1].reg.index)
				goto unsupported;
			word = (instruction.opcode == ARM32Opcode::Add ?
				UINT32_C(0x3000) : UINT32_C(0x3800)) |
				a[0].reg.index << 8 | static_cast<uint32_t>(immediate);
		}
		else goto unsupported;
	}
	else if (info.format == ARM32EncodingFormat::Compare) {
		if (instruction.opcode != ARM32Opcode::Cmp || !low_register(a[0]))
			goto unsupported;
		if (a[1].kind == ARM32OperandKind::Register && low_register(a[1]))
			word = UINT32_C(0x4280) | a[1].reg.index << 3 | a[0].reg.index;
		else if (a[1].kind == ARM32OperandKind::Immediate) {
			int64_t immediate;
			if (!absolute(a[1].value, immediate) || immediate < 0 ||
				immediate > 255) goto unsupported;
			word = UINT32_C(0x2800) | a[0].reg.index << 8 |
				static_cast<uint32_t>(immediate);
		}
		else goto unsupported;
	}
	else if (info.format == ARM32EncodingFormat::DataProcessing &&
		instruction.opcode == ARM32Opcode::Orr) {
		if (!instruction.set_flags || !low_register(a[0]) || !low_register(a[2]) ||
			a[0].reg.index != a[1].reg.index) goto unsupported;
		word = UINT32_C(0x4300) | a[2].reg.index << 3 |
			a[0].reg.index;
	}
	else if ((info.format == ARM32EncodingFormat::LoadStore) &&
		(instruction.opcode == ARM32Opcode::Ldr ||
		 instruction.opcode == ARM32Opcode::Str)) {
		if (a[0].kind != ARM32OperandKind::Register ||
			a[0].reg.index > 12 ||
			a[1].kind != ARM32OperandKind::Address ||
			a[1].reg.index > 12 ||
			a[1].address_kind != ARM32AddressKind::RegisterRelative)
			goto unsupported;
		int64_t immediate;
		if (!absolute(a[1].value, immediate) || immediate < 0 ||
			immediate > 4095 || (immediate & 3)) goto unsupported;
		if (low_register(a[0]) && a[1].reg.index < 8 && immediate <= 124) {
			word = (instruction.opcode == ARM32Opcode::Ldr ?
				UINT32_C(0x6800) : UINT32_C(0x6000)) |
				(static_cast<uint32_t>(immediate) >> 2) << 6 |
				a[1].reg.index << 3 | a[0].reg.index;
		}
		else {
			uint32_t first_halfword =
				(instruction.opcode == ARM32Opcode::Ldr ?
					UINT32_C(0xf8d0) : UINT32_C(0xf8c0)) |
				a[1].reg.index;
			uint32_t second_halfword = a[0].reg.index << 12 |
				static_cast<uint32_t>(immediate);
			word = first_halfword | second_halfword << 16;
			encoded.size = 4;
		}
	}
	else if (info.format == ARM32EncodingFormat::RegisterBranch &&
		instruction.opcode == ARM32Opcode::Bx) {
		if (a[0].kind != ARM32OperandKind::Register || a[0].reg.index > 15)
			goto unsupported;
		word = UINT32_C(0x4700) | a[0].reg.index << 3;
	}
	else if (info.format == ARM32EncodingFormat::Branch &&
		instruction.opcode == ARM32Opcode::B) {
		int64_t displacement = 0;
		TargetResolution resolution;
		if (a[0].value.wrt != NO_SEG) resolution = TargetResolution::Deferred;
		else if (a[0].value.unknown) resolution = TargetResolution::Deferred;
		else if (a[0].value.external ||
			(a[0].value.section != NO_SEG &&
			 a[0].value.section != session.Location.segment))
			resolution = TargetResolution::Deferred;
		else {
			displacement = a[0].value.offset - (offset + 4);
			resolution = TargetResolution::Resolved;
		}
		if (resolution == TargetResolution::Error) return false;
		bool conditional = instruction.condition != ARM32Condition::AL;
		unsigned bits = conditional ? 9 : 12;
		if (resolution == TargetResolution::Deferred) {
			encoded.fixup = ARM32FixupKind::Branch;
			encoded.target = a[0].value;
			encoded.size = 2;
			encoded.encoding = conditional ?
				(static_cast<uint32_t>(instruction.condition) << 8 | 0xd000) :
				0xe000;
			result.instructions[result.count++] = encoded;
			return true;
		}
		if ((displacement & 1) || !SignedRange(displacement, bits)) {
			aasm_log(_LOG_ERROR, "T32 branch target is unaligned or out of range");
			return false;
		}
		word = conditional ?
			(static_cast<uint32_t>(instruction.condition) << 8 | 0xd000) : 0xe000;
		word |= static_cast<uint32_t>(displacement >> 1) &
			(conditional ? 0xff : 0x7ff);
	}
	else goto unsupported;
	if (info.format != ARM32EncodingFormat::Branch &&
		instruction.condition != ARM32Condition::AL) goto unsupported;

	encoded.encoding = word;
	result.instructions[result.count++] = encoded;
	return true;

unsupported:
	aasm_log(_LOG_ERROR,
		"instruction form is not supported for the selected ARM32 instruction set");
	return false;
}

}

bool ARM32Encoder::Measure(const AssemblySession& session,
	const ARM32Instruction& instruction, int64_t offset, unsigned& size)
{
	ARM32EncodingSequence sequence;
	bool encoded = session.CurrentInstructionSet ==
		static_cast<uint32_t>(ARM32InstructionSet::T32) ?
		EncodeT32(session, instruction, offset, sequence) :
		EncodeOne(session, instruction, offset, sequence);
	if (!encoded) return false;
	size = 0;
	for (unsigned index = 0; index < sequence.count; index++)
		size += sequence.instructions[index].size;
	return true;
}

bool ARM32Encoder::Encode(const AssemblySession& session,
	const ARM32Instruction& instruction,
	int64_t offset, ARM32EncodingSequence& result)
{
	return session.CurrentInstructionSet ==
		static_cast<uint32_t>(ARM32InstructionSet::T32) ?
		EncodeT32(session, instruction, offset, result) :
		EncodeOne(session, instruction, offset, result);
}

}
