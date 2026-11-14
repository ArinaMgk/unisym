// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM64 Operand Model
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#ifndef UNISYM_LANGO_ARCHEND_ARM64_OPERAND_HPP
#define UNISYM_LANGO_ARCHEND_ARM64_OPERAND_HPP

#include "../../../../c/stdinc.h"

namespace uni::lango {

enum class ARM64RegisterKind : uint8_t {
	General,
	StackPointer,
	Zero
};

struct ARM64Register {
	ARM64RegisterKind kind = ARM64RegisterKind::General;
	uint8_t index = 0;
	uint8_t width = 64;
};

enum class ARM64Condition : uint8_t {
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
	None = 16
};

bool ParseARM64Register(const char* text, ARM64Register& result);
bool ParseARM64Condition(const char* text, ARM64Condition& result);

}

#endif
