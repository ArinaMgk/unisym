// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM32 Instruction Representation
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_ARM32_INSTRUCTION_HPP
#define UNISYM_LANGO_ARCHEND_ARM32_INSTRUCTION_HPP

#include "../../../../../asm/inc/aasm-limits.h"
#include "../../Archend.hpp"
#include "Operand-arm32.hpp"
#include "data/Instructions-arm32.hpp"

struct AssemblySession;

namespace uni::lango {

enum class ARM32OperandKind : uint8_t {
	None,
	Register,
	Immediate,
	Target,
	Address
};

enum class ARM32AddressKind : uint8_t {
	RegisterRelative,
	Expression
};

enum class ARM32InstructionKind : uint8_t {
	Native,
	MoveOperand,
	MoveLoad,
	MoveStore
};

struct ARM32Value {
	int64_t offset = 0;
	int32_t section = NO_SEG;
	int32_t wrt = NO_SEG;
	bool unknown = false;
	bool external = false;
};

struct ARM32Shift {
	ARM32ShiftKind kind = ARM32ShiftKind::None;
	ARM32Value amount;
};

struct ARM32Operand {
	ARM32OperandKind kind = ARM32OperandKind::None;
	ARM32Register reg;
	ARM32AddressKind address_kind = ARM32AddressKind::RegisterRelative;
	ARM32Value value;
	ARM32Shift shift;
	uint8_t width = 0;
};

struct ARM32Instruction {
	ARM32InstructionKind kind = ARM32InstructionKind::Native;
	ARM32Opcode opcode = ARM32Opcode::None;
	ARM32Condition condition = ARM32Condition::AL;
	AssemblySignedness signedness = AssemblySignedness::Default;
	ARM32Operand operands[4];
	unsigned operand_count = 0;
	bool set_flags = false;
};

enum class ARM32FixupKind : uint8_t {
	None,
	Branch,
	Call,
	AddressLow,
	AddressHigh
};

enum class ARM32FixupPairRole : uint8_t {
	None,
	Head,
	Tail
};

struct ARM32EncodingResult {
	uint32_t encoding = 0;
	ARM32FixupKind fixup = ARM32FixupKind::None;
	ARM32Value target;
	uint64_t pair_offset = 0;
	ARM32FixupPairRole pair_role = ARM32FixupPairRole::None;
};

struct ARM32EncodingSequence {
	ARM32EncodingResult instructions[2];
	unsigned count = 0;
};

class ARM32Parser {
public:
	static bool IsInstructionStart(const char* word);
	static bool Parse(AssemblySession& session, char* source,
		int parse_pass, ARM32Instruction& instruction);
};

class ARM32Encoder {
public:
	static bool Measure(const AssemblySession& session,
		const ARM32Instruction& instruction, int64_t offset, unsigned& size);
	static bool Encode(const AssemblySession& session,
		const ARM32Instruction& instruction,
		int64_t offset, ARM32EncodingSequence& result);
};

}

#endif
