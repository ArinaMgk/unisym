// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Vector Operand Support
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_RISCV_VECTOR_HPP
#define UNISYM_LANGO_ARCHEND_RISCV_VECTOR_HPP

#include "Operand-riscv.hpp"

namespace uni::lango {

bool ParseRISCVVectorType(char* operands[], unsigned start, unsigned count,
	RISCVVectorType& type);
bool ParseRISCVVectorMask(char* text, bool& masked);
uint32_t EncodeRISCVVectorType(const RISCVVectorType& type);

}

#endif
