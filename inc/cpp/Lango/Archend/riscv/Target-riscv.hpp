// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Target Description
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_RISCV_TARGET_HPP
#define UNISYM_LANGO_ARCHEND_RISCV_TARGET_HPP

#include "../../../../c/stdinc.h"

namespace uni::lango {

enum class RISCVExtension : uint32_t {
	None = 0,
	I = UINT32_C(1) << 0,
	M = UINT32_C(1) << 1,
	A = UINT32_C(1) << 2,
	F = UINT32_C(1) << 3,
	D = UINT32_C(1) << 4,
	C = UINT32_C(1) << 5,
	V = UINT32_C(1) << 6,
	Zicsr = UINT32_C(1) << 7,
	Zifencei = UINT32_C(1) << 8
};

constexpr uint32_t RISCVExtensionMask(RISCVExtension extension)
{
	return static_cast<uint32_t>(extension);
}

enum class RISCVABI : uint32_t {
	Default,
	ILP32,
	ILP32F,
	ILP32D,
	LP64,
	LP64F,
	LP64D
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
