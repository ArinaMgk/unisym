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
	None,           // Operand is absent.
	Register,       // Integer, floating-point, or vector register.
	Immediate,      // Absolute or relocatable expression value.
	CSR,            // Control and status register number.
	Target,         // Direct control-flow target.
	IndirectTarget, // Register-relative indirect control-flow target.
	Address         // Memory address operand.
};

enum class RISCVAddressKind : uint8_t {
	RegisterRelative, // Immediate displacement relative to a base register.
	Expression        // Standalone address expression requiring expansion.
};

enum class RISCVInstructionKind : uint8_t {
	Native,         // One instruction described by the generated instruction table.
	MoveExpression, // Semantic MOV of an immediate value or symbol address.
	LoadImmediate,  // LI pseudo-instruction with an absolute expression.
	LoadAddress,    // LA pseudo-instruction with an address expression.
	Call,           // CALL expansion using the standard call relocation.
	CallPlt,        // CALL_PLT expansion using the PLT call relocation.
	Tail,           // TAIL expansion using t1 as scratch and zero as link.
	IntegerExtend   // Extension-aware integer sign or zero extension sequence.
};

struct RISCVIntegerExtend {
	AssemblySignedness signedness = AssemblySignedness::Default; // Result extension kind.
	uint8_t source_bytes = 0; // Source value width before extension.
};

enum class RISCVRelocationKind : uint8_t {
	None,           // No explicit relocation modifier.
	High,           // Absolute high 20 bits.
	Low,            // Absolute low 12 bits.
	Got,            // Global-offset-table address.
	PcRelativeHigh, // PC-relative high 20 bits.
	PcRelativeLow   // PC-relative low 12 bits paired with a high relocation.
};

enum class RISCVAtomicOrdering : uint8_t {
	None = 0,    // No acquire or release ordering bit.
	Acquire = 1, // Set the aq bit.
	Release = 2  // Set the rl bit.
};

enum class RISCVFenceAccess : uint8_t {
	Write = 1,  // Memory writes.
	Read = 2,   // Memory reads.
	Output = 4, // Device output operations.
	Input = 8   // Device input operations.
};

struct RISCVFenceOptions {
	uint8_t mode = 0;
	uint8_t predecessor = 15;
	uint8_t successor = 15;
};

struct RISCVValue {
	int64_t offset = 0;
	int32_t section = NO_SEG;
	int32_t wrt = NO_SEG;
	bool unknown = false;
	bool external = false;
	bool symbol = false;
	bool global = false;
	int64_t symbol_offset = 0;
	int64_t addend = 0;
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
	RISCVFenceOptions fence;
	RISCVIntegerExtend integer_extend;
	uint8_t temporary_reg = 0;
	bool has_temporary = false;
	bool masked = false;
	RISCVOperand operands[5];
	unsigned operand_count = 0;
};

enum class RISCVFixupKind : uint8_t {
	None,             // Encoding needs no linker fixup.
	Branch,           // B-type PC-relative branch relocation.
	Jump,             // J-type PC-relative jump relocation.
	CompressedBranch, // Compressed branch relocation.
	CompressedJump,   // Compressed jump relocation.
	Call,             // AUIPC/JALR direct call relocation pair.
	CallPlt,          // AUIPC/JALR PLT call relocation pair.
	High,             // Absolute U-type high relocation.
	LowI,             // Absolute I-type low relocation.
	LowS,             // Absolute S-type low relocation.
	GotHigh,          // GOT-relative high relocation.
	PcRelativeHigh,   // PC-relative high relocation.
	PcRelativeLowI,   // PC-relative I-type low relocation.
	PcRelativeLowS    // PC-relative S-type low relocation.
};

enum class RISCVFixupPairRole : uint8_t {
	None, // Fixup is independent.
	Head, // First instruction of a paired relocation.
	Tail  // Second instruction of a paired relocation.
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
