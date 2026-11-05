// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Instruction Representation
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_RISCV_INSTRUCTION_HPP
#define UNISYM_LANGO_ARCHEND_RISCV_INSTRUCTION_HPP

#include "../../../../../asm/inc/aasm-limits.h"
#include "../../Archend.hpp"

struct AssemblySession;

namespace uni::lango {

enum class RISCVOpcode : uint8_t {
	None,
	Lui, Auipc, Jal, Jalr,
	Beq, Bne, Blt, Bge, Bltu, Bgeu,
	Lb, Lh, Lw, Lbu, Lhu, Lwu, Ld,
	Sb, Sh, Sw, Sd,
	Addi, Slti, Sltiu, Xori, Ori, Andi, Slli, Srli, Srai,
	Add, Sub, Sll, Slt, Sltu, Xor, Srl, Sra, Or, And,
	Addiw, Slliw, Srliw, Sraiw,
	Addw, Subw, Sllw, Srlw, Sraw,
	Fence, Ecall, Ebreak
};

enum class RISCVOperandKind : uint8_t {
	None,
	Register,
	Immediate,
	Address
};

struct RISCVValue {
	int64_t offset = 0;
	int32_t section = NO_SEG;
	int32_t wrt = NO_SEG;
	bool unknown = false;
	bool external = false;
};

struct RISCVOperand {
	RISCVOperandKind kind = RISCVOperandKind::None;
	uint8_t reg = 0;
	uint8_t width = 0;
	RISCVValue value;
};

struct RISCVInstruction {
	RISCVOpcode opcode = RISCVOpcode::None;
	RISCVOperand operands[3];
	unsigned operand_count = 0;
};

class RISCVParser {
public:
	static bool IsInstructionStart(const char* word);
	static bool Parse(AssemblySession& session, char* source,
		int parse_pass, RISCVInstruction& instruction);
};

class RISCVEncoder {
public:
	static bool Encode(const AssemblySession& session,
		const RISCVInstruction& instruction,
		int64_t offset, uint32_t& encoding);
};

}

#endif
