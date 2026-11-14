// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM64 Instruction Representation
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_ARM64_INSTRUCTION_HPP
#define UNISYM_LANGO_ARCHEND_ARM64_INSTRUCTION_HPP

#include "../../../../../asm/inc/aasm-limits.h"
#include "../../Archend.hpp"
#include "Operand-arm64.hpp"
#include "data/Instructions-arm64.hpp"

struct AssemblySession;

namespace uni::lango {

enum class ARM64OperandKind : uint8_t {
	None,
	Register,
	Immediate,
	Target,
	Address
};

enum class ARM64AddressKind : uint8_t {
	RegisterRelative,
	Expression
};

enum class ARM64InstructionKind : uint8_t {
	Native,
	ConditionalBranch,
	MoveRegister,
	MoveImmediate,
	MoveLoad,
	MoveStore
};

struct ARM64Value {
	int64_t offset = 0;
	int32_t section = NO_SEG;
	int32_t wrt = NO_SEG;
	bool unknown = false;
	bool external = false;
};

struct ARM64Operand {
	ARM64OperandKind kind = ARM64OperandKind::None;
	ARM64Register reg;
	ARM64AddressKind address_kind = ARM64AddressKind::RegisterRelative;
	ARM64Value value;
	uint8_t width = 0;
};

struct ARM64Instruction {
	ARM64InstructionKind kind = ARM64InstructionKind::Native;
	ARM64Opcode opcode = ARM64Opcode::None;
	ARM64Condition condition = ARM64Condition::None;
	AssemblySignedness signedness = AssemblySignedness::Default;
	ARM64Operand operands[4];
	unsigned operand_count = 0;
};

enum class ARM64FixupKind : uint8_t {
	None,
	Branch,
	ConditionalBranch,
	Call,
	PageRelative,
	PageOffset
};

enum class ARM64FixupPairRole : uint8_t {
	None,
	Head,
	Tail
};

struct ARM64EncodingResult {
	uint32_t encoding = 0;
	ARM64FixupKind fixup = ARM64FixupKind::None;
	ARM64Value target;
	uint64_t pair_offset = 0;
	ARM64FixupPairRole pair_role = ARM64FixupPairRole::None;
};

struct ARM64EncodingSequence {
	ARM64EncodingResult instructions[4];
	unsigned count = 0;
};

class ARM64Parser {
public:
	static bool IsInstructionStart(const char* word);
	static bool Parse(AssemblySession& session, char* source,
		int parse_pass, ARM64Instruction& instruction);
};

class ARM64Encoder {
public:
	static bool Measure(const AssemblySession& session,
		const ARM64Instruction& instruction, int64_t offset, unsigned& size);
	static bool Encode(const AssemblySession& session,
		const ARM64Instruction& instruction,
		int64_t offset, ARM64EncodingSequence& result);
};

}

#endif
