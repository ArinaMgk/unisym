// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Target Description
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_RISCV_TARGET_HPP
#define UNISYM_LANGO_ARCHEND_RISCV_TARGET_HPP

#include "../../../../c/stdinc.h"

namespace uni::lango {

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
bool ParseRISCVABI(const char* name, RISCVABI& abi);
bool ValidateRISCVABI(int bits, uint32_t extensions, RISCVABI abi);
uint32_t RISCVABIFlags(RISCVABI abi);
bool FormatRISCVArchitecture(int bits, uint32_t extensions,
	char* output, size_t capacity);

}

#endif
