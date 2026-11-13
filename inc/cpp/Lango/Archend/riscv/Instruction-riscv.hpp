// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Instruction Representation
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_RISCV_INSTRUCTION_HPP
#define UNISYM_LANGO_ARCHEND_RISCV_INSTRUCTION_HPP

#include "../../../../../asm/inc/aasm-limits.h"
#include "../../Archend.hpp"
#include "data/Instructions-riscv.hpp"
#include "Operand-riscv.hpp"

struct AssemblySession;

namespace uni::lango {

enum class RISCVOperandKind : uint8_t {
	None,
	Register,
	Immediate,
	CSR,
	Target,
	IndirectTarget,
	Address
};

enum class RISCVAddressKind : uint8_t {
	RegisterRelative,
	Expression
};

enum class RISCVInstructionKind : uint8_t {
	Native,
	MoveExpression,
	Call,
	CallPlt
};

enum class RISCVRelocationKind : uint8_t {
	None,
	High,
	Low,
	Got,
	PcRelativeHigh,
	PcRelativeLow
};

enum class RISCVAtomicOrdering : uint8_t {
	None = 0,
	Acquire = 1,
	Release = 2
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
	RISCVAddressKind address_kind = RISCVAddressKind::RegisterRelative;
	uint8_t reg = 0;
	RISCVRegisterClass reg_class = RISCVRegisterClass::Integer;
	uint8_t width = 0;
	RISCVRelocationKind relocation = RISCVRelocationKind::None;
	RISCVValue value;
};

struct RISCVInstruction {
	RISCVInstructionKind kind = RISCVInstructionKind::Native;
	RISCVOpcode opcode = RISCVOpcode::None;
	AssemblySignedness signedness = AssemblySignedness::Default;
	RISCVAtomicOrdering atomic_ordering = RISCVAtomicOrdering::None;
	RISCVRoundingMode rounding = RISCVRoundingMode::DYN;
	RISCVVectorType vector_type;
	uint8_t temporary_reg = 0;
	bool has_temporary = false;
	bool masked = false;
	RISCVOperand operands[5];
	unsigned operand_count = 0;
};

enum class RISCVFixupKind : uint8_t {
	None,
	Branch,
	Jump,
	CompressedBranch,
	CompressedJump,
	Call,
	CallPlt,
	High,
	LowI,
	LowS,
	GotHigh,
	PcRelativeHigh,
	PcRelativeLowI,
	PcRelativeLowS
};

enum class RISCVFixupPairRole : uint8_t {
	None,
	Head,
	Tail
};

struct RISCVEncodingResult {
	uint64_t encoding = 0;
	uint8_t size = 4;
	RISCVFixupKind fixup = RISCVFixupKind::None;
	RISCVValue target;
	uint64_t pair_offset = 0;
	RISCVFixupPairRole pair_role = RISCVFixupPairRole::None;
};

struct RISCVEncodingSequence {
	RISCVEncodingResult instructions[10];
	unsigned count = 0;
};

class RISCVParser {
public:
	static bool IsInstructionStart(const char* word);
	static bool Parse(AssemblySession& session, char* source,
		int parse_pass, RISCVInstruction& instruction);
};

class RISCVEncoder {
public:
	static bool Measure(const AssemblySession& session,
		const RISCVInstruction& instruction, int64_t offset, unsigned& count);
	static bool Encode(const AssemblySession& session,
		const RISCVInstruction& instruction,
		int64_t offset, RISCVEncodingSequence& result);
};

}

#endif
