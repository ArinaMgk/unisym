// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Operand Model
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_RISCV_OPERAND_HPP
#define UNISYM_LANGO_ARCHEND_RISCV_OPERAND_HPP

#include "../../../../c/stdinc.h"

namespace uni::lango {

enum class RISCVRegisterClass : uint8_t {
	Integer,
	Float,
	Vector
};

struct RISCVRegister {
	RISCVRegisterClass kind = RISCVRegisterClass::Integer;
	uint8_t index = 0;
};

enum class RISCVRoundingMode : uint8_t {
	RNE = 0,
	RTZ = 1,
	RDN = 2,
	RUP = 3,
	RMM = 4,
	DYN = 7
};

struct RISCVVectorType {
	uint16_t element_bits = 8;
	int8_t lmul = 0;
	bool tail_agnostic = true;
	bool mask_agnostic = true;
};

bool ParseRISCVRegister(const char* text, RISCVRegister& result);
bool ParseRISCVCSR(const char* text, uint16_t& result);
bool ParseRISCVRoundingMode(const char* text, RISCVRoundingMode& result);

}

#endif
