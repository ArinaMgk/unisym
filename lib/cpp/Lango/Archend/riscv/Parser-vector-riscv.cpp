// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Vector Operand Parser
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/riscv/Vector-riscv.hpp"

namespace uni::lango {

namespace {

char* SkipSpace(char* text)
{
	while (*text && ascii_isspace(*text)) text++;
	return text;
}

void TrimEnd(char* text)
{
	size_t length = StrLength(text);
	while (length && ascii_isspace(text[length - 1])) text[--length] = '\0';
}

bool Equal(const char* left, const char* right)
{
	return !StrCompareInsensitive(left, right);
}

}

bool ParseRISCVVectorType(char* operands[], unsigned start, unsigned count,
	RISCVVectorType& type)
{
	bool have_element = false, have_lmul = false;
	for (unsigned index = start; index < count; index++) {
		char* text = SkipSpace(operands[index]);
		TrimEnd(text);
		if ((text[0] == 'e' || text[0] == 'E') && text[1]) {
			char* end;
			long bits = StrTokenLong(text + 1, &end, 10);
			if (*end || (bits != 8 && bits != 16 && bits != 32 && bits != 64) ||
				have_element) return false;
			type.element_bits = static_cast<uint16_t>(bits);
			have_element = true;
			continue;
		}
		if (Equal(text, "m1")) { if (have_lmul) return false; type.lmul = 0; have_lmul = true; continue; }
		if (Equal(text, "m2")) { if (have_lmul) return false; type.lmul = 1; have_lmul = true; continue; }
		if (Equal(text, "m4")) { if (have_lmul) return false; type.lmul = 2; have_lmul = true; continue; }
		if (Equal(text, "m8")) { if (have_lmul) return false; type.lmul = 3; have_lmul = true; continue; }
		if (Equal(text, "mf2")) { if (have_lmul) return false; type.lmul = 7; have_lmul = true; continue; }
		if (Equal(text, "mf4")) { if (have_lmul) return false; type.lmul = 6; have_lmul = true; continue; }
		if (Equal(text, "mf8")) { if (have_lmul) return false; type.lmul = 5; have_lmul = true; continue; }
		if (Equal(text, "ta")) { type.tail_agnostic = true; continue; }
		if (Equal(text, "tu")) { type.tail_agnostic = false; continue; }
		if (Equal(text, "ma")) { type.mask_agnostic = true; continue; }
		if (Equal(text, "mu")) { type.mask_agnostic = false; continue; }
		return false;
	}
	return have_element && have_lmul;
}

bool ParseRISCVVectorMask(char* text, bool& masked)
{
	text = SkipSpace(text);
	TrimEnd(text);
	if (!Equal(text, "v0.t")) return false;
	masked = true;
	return true;
}

}
