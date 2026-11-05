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

uint32_t EncodeR(uint32_t opcode, uint32_t funct3, uint32_t funct7,
	uint32_t rd, uint32_t rs1, uint32_t rs2)
{
	return opcode | rd << 7 | funct3 << 12 | rs1 << 15 |
		rs2 << 20 | funct7 << 25;
}

uint32_t EncodeI(uint32_t opcode, uint32_t funct3,
	uint32_t rd, uint32_t rs1, int64_t immediate)
{
	return opcode | rd << 7 | funct3 << 12 | rs1 << 15 |
		(static_cast<uint32_t>(immediate) & 0xfff) << 20;
}

uint32_t EncodeS(uint32_t funct3, uint32_t rs1, uint32_t rs2,
	int64_t immediate)
{
	uint32_t value = static_cast<uint32_t>(immediate) & 0xfff;
	return UINT32_C(0x23) | (value & 0x1f) << 7 | funct3 << 12 |
		rs1 << 15 | rs2 << 20 | (value >> 5) << 25;
}

uint32_t EncodeB(uint32_t funct3, uint32_t rs1, uint32_t rs2,
	int64_t immediate)
{
	uint32_t value = static_cast<uint32_t>(immediate) & 0x1fff;
	return UINT32_C(0x63) | ((value >> 11) & 1) << 7 |
		((value >> 1) & 0xf) << 8 | funct3 << 12 | rs1 << 15 |
		rs2 << 20 | ((value >> 5) & 0x3f) << 25 |
		((value >> 12) & 1) << 31;
}

uint32_t EncodeJ(uint32_t rd, int64_t immediate)
{
	uint32_t value = static_cast<uint32_t>(immediate) & 0x1fffff;
	return UINT32_C(0x6f) | rd << 7 | ((value >> 12) & 0xff) << 12 |
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

bool RequireRV64(const AssemblySession& session)
{
	if (session.CurrentBits == 64) return true;
	aasm_log(_LOG_ERROR, "RISC-V instruction requires 64-bit mode");
	return false;
}

}

bool RISCVEncoder::Encode(const AssemblySession& session,
	const RISCVInstruction& instruction, int64_t offset, uint32_t& encoding)
{
	const RISCVOperand* operand = instruction.operands;
	int64_t immediate;
	uint32_t funct3;
	uint32_t funct7;

	switch (instruction.opcode) {
	case RISCVOpcode::Lui:
	case RISCVOpcode::Auipc:
		if (!RequireImmediate(operand[1].value, immediate)) return false;
		if (immediate < INT64_C(-0x80000) || immediate > INT64_C(0xfffff)) {
			aasm_log(_LOG_ERROR, "RISC-V upper immediate is outside 20 bits");
			return false;
		}
		encoding = (instruction.opcode == RISCVOpcode::Lui ?
			UINT32_C(0x37) : UINT32_C(0x17)) |
			static_cast<uint32_t>(operand[0].reg) << 7 |
			(static_cast<uint32_t>(immediate) & 0xfffff) << 12;
		return true;

	case RISCVOpcode::Jal:
		if (!ResolveTarget(session, operand[1].value, offset, immediate)) return false;
		if ((immediate & 1) || !SignedRange(immediate, 21)) {
			aasm_log(_LOG_ERROR, "RISC-V jump target is outside the J range");
			return false;
		}
		encoding = EncodeJ(operand[0].reg, immediate);
		return true;

	case RISCVOpcode::Jalr:
		if (!RequireIImmediate(operand[1], immediate)) return false;
		encoding = EncodeI(0x67, 0, operand[0].reg, operand[1].reg, immediate);
		return true;

	case RISCVOpcode::Beq:
	case RISCVOpcode::Bne:
	case RISCVOpcode::Blt:
	case RISCVOpcode::Bge:
	case RISCVOpcode::Bltu:
	case RISCVOpcode::Bgeu:
		switch (instruction.opcode) {
		case RISCVOpcode::Beq: funct3 = 0; break;
		case RISCVOpcode::Bne: funct3 = 1; break;
		case RISCVOpcode::Blt: funct3 = 4; break;
		case RISCVOpcode::Bge: funct3 = 5; break;
		case RISCVOpcode::Bltu: funct3 = 6; break;
		default: funct3 = 7; break;
		}
		if (!ResolveTarget(session, operand[2].value, offset, immediate)) return false;
		if ((immediate & 1) || !SignedRange(immediate, 13)) {
			aasm_log(_LOG_ERROR, "RISC-V branch target is outside the B range");
			return false;
		}
		encoding = EncodeB(funct3, operand[0].reg, operand[1].reg, immediate);
		return true;

	case RISCVOpcode::Lb:
	case RISCVOpcode::Lh:
	case RISCVOpcode::Lw:
	case RISCVOpcode::Lbu:
	case RISCVOpcode::Lhu:
	case RISCVOpcode::Lwu:
	case RISCVOpcode::Ld:
		if ((instruction.opcode == RISCVOpcode::Lwu ||
			instruction.opcode == RISCVOpcode::Ld) && !RequireRV64(session)) return false;
		switch (instruction.opcode) {
		case RISCVOpcode::Lb: funct3 = 0; break;
		case RISCVOpcode::Lh: funct3 = 1; break;
		case RISCVOpcode::Lw: funct3 = 2; break;
		case RISCVOpcode::Ld: funct3 = 3; break;
		case RISCVOpcode::Lbu: funct3 = 4; break;
		case RISCVOpcode::Lhu: funct3 = 5; break;
		default: funct3 = 6; break;
		}
		if (!RequireIImmediate(operand[1], immediate)) return false;
		encoding = EncodeI(0x03, funct3, operand[0].reg, operand[1].reg, immediate);
		return true;

	case RISCVOpcode::Sb:
	case RISCVOpcode::Sh:
	case RISCVOpcode::Sw:
	case RISCVOpcode::Sd:
		if (instruction.opcode == RISCVOpcode::Sd && !RequireRV64(session)) return false;
		funct3 = instruction.opcode == RISCVOpcode::Sb ? 0 :
			instruction.opcode == RISCVOpcode::Sh ? 1 :
			instruction.opcode == RISCVOpcode::Sw ? 2 : 3;
		if (!RequireIImmediate(operand[1], immediate)) return false;
		encoding = EncodeS(funct3, operand[1].reg, operand[0].reg, immediate);
		return true;

	case RISCVOpcode::Addi:
	case RISCVOpcode::Slti:
	case RISCVOpcode::Sltiu:
	case RISCVOpcode::Xori:
	case RISCVOpcode::Ori:
	case RISCVOpcode::Andi:
		funct3 = instruction.opcode == RISCVOpcode::Addi ? 0 :
			instruction.opcode == RISCVOpcode::Slti ? 2 :
			instruction.opcode == RISCVOpcode::Sltiu ? 3 :
			instruction.opcode == RISCVOpcode::Xori ? 4 :
			instruction.opcode == RISCVOpcode::Ori ? 6 : 7;
		if (!RequireIImmediate(operand[2], immediate)) return false;
		encoding = EncodeI(0x13, funct3, operand[0].reg, operand[1].reg, immediate);
		return true;

	case RISCVOpcode::Slli:
	case RISCVOpcode::Srli:
	case RISCVOpcode::Srai:
		if (!RequireImmediate(operand[2].value, immediate)) return false;
		if (immediate < 0 || immediate >= session.CurrentBits) {
			aasm_log(_LOG_ERROR, "RISC-V shift amount is outside the register width");
			return false;
		}
		funct3 = instruction.opcode == RISCVOpcode::Slli ? 1 : 5;
		if (instruction.opcode == RISCVOpcode::Srai) immediate |= UINT32_C(0x400);
		encoding = EncodeI(0x13, funct3, operand[0].reg, operand[1].reg, immediate);
		return true;

	case RISCVOpcode::Add:
	case RISCVOpcode::Sub:
	case RISCVOpcode::Sll:
	case RISCVOpcode::Slt:
	case RISCVOpcode::Sltu:
	case RISCVOpcode::Xor:
	case RISCVOpcode::Srl:
	case RISCVOpcode::Sra:
	case RISCVOpcode::Or:
	case RISCVOpcode::And:
		funct3 = instruction.opcode == RISCVOpcode::Add ||
			instruction.opcode == RISCVOpcode::Sub ? 0 :
			instruction.opcode == RISCVOpcode::Sll ? 1 :
			instruction.opcode == RISCVOpcode::Slt ? 2 :
			instruction.opcode == RISCVOpcode::Sltu ? 3 :
			instruction.opcode == RISCVOpcode::Xor ? 4 :
			instruction.opcode == RISCVOpcode::Srl ||
			instruction.opcode == RISCVOpcode::Sra ? 5 :
			instruction.opcode == RISCVOpcode::Or ? 6 : 7;
		funct7 = instruction.opcode == RISCVOpcode::Sub ||
			instruction.opcode == RISCVOpcode::Sra ? 0x20 : 0;
		encoding = EncodeR(0x33, funct3, funct7, operand[0].reg,
			operand[1].reg, operand[2].reg);
		return true;

	case RISCVOpcode::Addiw:
		if (!RequireRV64(session) || !RequireIImmediate(operand[2], immediate))
			return false;
		encoding = EncodeI(0x1b, 0, operand[0].reg, operand[1].reg, immediate);
		return true;

	case RISCVOpcode::Slliw:
	case RISCVOpcode::Srliw:
	case RISCVOpcode::Sraiw:
		if (!RequireRV64(session) ||
			!RequireImmediate(operand[2].value, immediate)) return false;
		if (immediate < 0 || immediate > 31) {
			aasm_log(_LOG_ERROR, "RISC-V word shift amount is outside 5 bits");
			return false;
		}
		funct3 = instruction.opcode == RISCVOpcode::Slliw ? 1 : 5;
		if (instruction.opcode == RISCVOpcode::Sraiw) immediate |= UINT32_C(0x400);
		encoding = EncodeI(0x1b, funct3, operand[0].reg, operand[1].reg, immediate);
		return true;

	case RISCVOpcode::Addw:
	case RISCVOpcode::Subw:
	case RISCVOpcode::Sllw:
	case RISCVOpcode::Srlw:
	case RISCVOpcode::Sraw:
		if (!RequireRV64(session)) return false;
		funct3 = instruction.opcode == RISCVOpcode::Addw ||
			instruction.opcode == RISCVOpcode::Subw ? 0 :
			instruction.opcode == RISCVOpcode::Sllw ? 1 : 5;
		funct7 = instruction.opcode == RISCVOpcode::Subw ||
			instruction.opcode == RISCVOpcode::Sraw ? 0x20 : 0;
		encoding = EncodeR(0x3b, funct3, funct7, operand[0].reg,
			operand[1].reg, operand[2].reg);
		return true;

	case RISCVOpcode::Fence:
		encoding = UINT32_C(0x0ff0000f);
		return true;
	case RISCVOpcode::Ecall:
		encoding = UINT32_C(0x00000073);
		return true;
	case RISCVOpcode::Ebreak:
		encoding = UINT32_C(0x00100073);
		return true;
	default:
		aasm_log(_LOG_ERROR, "RISC-V instruction encoding is not implemented");
		return false;
	}
}

}
