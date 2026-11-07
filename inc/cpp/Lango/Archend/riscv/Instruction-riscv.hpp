// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Instruction Representation
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_RISCV_INSTRUCTION_HPP
#define UNISYM_LANGO_ARCHEND_RISCV_INSTRUCTION_HPP

#include "../../../../../asm/inc/aasm-limits.h"
#include "../../Archend.hpp"
#include "data/Instructions-riscv.hpp"

struct AssemblySession;

namespace uni::lango {

enum class RISCVOperandKind : uint8_t {
	None,
	Register,
	Immediate,
	Target,
	IndirectTarget,
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
	AssemblySignedness signedness = AssemblySignedness::Default;
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
