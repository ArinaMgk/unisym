// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM32 Operand Model
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../inc/c/ustring.h"
#include "../../../../../inc/cpp/Lango/Archend/arm32/Operand-arm32.hpp"

namespace uni::lango {

namespace {

bool Equal(const char* left, const char* right)
{
	return !StrCompareInsensitive(left, right);
}

}

bool ParseARM32Register(const char* text, ARM32Register& result)
{
	if (Equal(text, "sp")) result.index = 13;
	else if (Equal(text, "lr")) result.index = 14;
	else if (Equal(text, "pc")) result.index = 15;
	else {
		if (ascii_tolower(text[0]) != 'r' || !text[1]) return false;
		char* end;
		long number = StrTokenLong(text + 1, &end, 10);
		if (*end || number < 0 || number > 15) return false;
		result.index = static_cast<uint8_t>(number);
	}
	return true;
}

bool ParseARM32Condition(const char* text, ARM32Condition& result)
{
	struct Entry {
		const char* name;
		ARM32Condition condition;
	};
	static const Entry entries[] = {
		{"eq", ARM32Condition::EQ}, {"ne", ARM32Condition::NE},
		{"cs", ARM32Condition::CS}, {"hs", ARM32Condition::CS},
		{"cc", ARM32Condition::CC}, {"lo", ARM32Condition::CC},
		{"mi", ARM32Condition::MI}, {"pl", ARM32Condition::PL},
		{"vs", ARM32Condition::VS}, {"vc", ARM32Condition::VC},
		{"hi", ARM32Condition::HI}, {"ls", ARM32Condition::LS},
		{"ge", ARM32Condition::GE}, {"lt", ARM32Condition::LT},
		{"gt", ARM32Condition::GT}, {"le", ARM32Condition::LE},
		{"al", ARM32Condition::AL}
	};
	for (const Entry& entry : entries)
		if (Equal(text, entry.name)) {
			result = entry.condition;
			return true;
		}
	return false;
}

bool ParseARM32Shift(const char* text, ARM32ShiftKind& result)
{
	if (Equal(text, "lsl")) result = ARM32ShiftKind::LSL;
	else if (Equal(text, "lsr")) result = ARM32ShiftKind::LSR;
	else if (Equal(text, "asr")) result = ARM32ShiftKind::ASR;
	else if (Equal(text, "ror")) result = ARM32ShiftKind::ROR;
	else return false;
	return true;
}

}
