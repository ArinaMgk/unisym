// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM32 Operand Model
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_ARM32_OPERAND_HPP
#define UNISYM_LANGO_ARCHEND_ARM32_OPERAND_HPP

#include "../../../../c/stdinc.h"

namespace uni::lango {

struct ARM32Register {
	uint8_t index = 0;
};

enum class ARM32Condition : uint8_t {
	EQ = 0,
	NE = 1,
	CS = 2,
	CC = 3,
	MI = 4,
	PL = 5,
	VS = 6,
	VC = 7,
	HI = 8,
	LS = 9,
	GE = 10,
	LT = 11,
	GT = 12,
	LE = 13,
	AL = 14,
	None = 16
};

enum class ARM32ShiftKind : uint8_t {
	None,
	LSL,
	LSR,
	ASR,
	ROR
};

bool ParseARM32Register(const char* text, ARM32Register& result);
bool ParseARM32Condition(const char* text, ARM32Condition& result);
bool ParseARM32Shift(const char* text, ARM32ShiftKind& result);

}

#endif
