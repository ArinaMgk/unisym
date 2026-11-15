// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Target Description
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_RISCV_TARGET_HPP
#define UNISYM_LANGO_ARCHEND_RISCV_TARGET_HPP

#include "../../../../c/stdinc.h"

namespace uni::lango {

enum class RISCVExtension : uint32_t {
	None = 0,                       // No instruction-set extension.
	I = UINT32_C(1) << 0,          // Base integer instruction set.
	M = UINT32_C(1) << 1,          // Integer multiplication and division.
	A = UINT32_C(1) << 2,          // Atomic memory operations.
	F = UINT32_C(1) << 3,          // Single-precision floating point.
	D = UINT32_C(1) << 4,          // Double-precision floating point.
	C = UINT32_C(1) << 5,          // Compressed instructions.
	V = UINT32_C(1) << 6,          // Vector instructions.
	Zba = UINT32_C(1) << 7,        // Address-generation bit operations.
	Zbb = UINT32_C(1) << 8,        // Basic bit-manipulation operations.
	Zbs = UINT32_C(1) << 9,        // Single-bit operations.
	Zicond = UINT32_C(1) << 10,    // Integer conditional operations.
	Zicsr = UINT32_C(1) << 11,     // Control and status register access.
	Zifencei = UINT32_C(1) << 12   // Instruction-fetch fence.
};

constexpr uint32_t RISCVExtensionMask(RISCVExtension extension)
{
	return static_cast<uint32_t>(extension);
}

enum class RISCVABI : uint32_t {
	Default, // Select the default ABI for the active XLEN and extensions.
	ILP32,   // 32-bit integer ABI with software floating-point calling convention.
	ILP32F,  // 32-bit integer ABI with single-precision floating-point arguments.
	ILP32D,  // 32-bit integer ABI with double-precision floating-point arguments.
	LP64,    // 64-bit integer ABI with software floating-point calling convention.
	LP64F,   // 64-bit integer ABI with single-precision floating-point arguments.
	LP64D    // 64-bit integer ABI with double-precision floating-point arguments.
};

RISCVABI DefaultRISCVABI(int bits);
bool ParseRISCVInstructionSet(const char* name, uint32_t& extensions);
bool ValidateRISCVInstructionSet(uint32_t extensions);
bool HasRISCVExtension(uint32_t extensions, RISCVExtension extension);
bool ParseRISCVABI(const char* name, RISCVABI& abi);
bool ValidateRISCVABI(int bits, uint32_t extensions, RISCVABI abi);
uint32_t RISCVABIFlags(RISCVABI abi);
bool FormatRISCVArchitecture(int bits, uint32_t extensions,
	char* output, size_t capacity);

}

#endif
