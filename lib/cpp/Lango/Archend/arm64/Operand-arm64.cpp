// ASCII C++11 TAB4 LF
// Docutitle: AASM ARM64 Operand Model
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../inc/c/ustring.h"
#include "../../../../../inc/cpp/Lango/Archend/arm64/Operand-arm64.hpp"

namespace uni::lango {

namespace {

bool Equal(const char* left, const char* right)
{
	return !StrCompareInsensitive(left, right);
}

bool ParseNumbered(const char* text, char prefix, uint8_t width,
	ARM64Register& result)
{
	if (ascii_tolower(text[0]) != prefix || !text[1]) return false;
	char* end;
	long number = StrTokenLong(text + 1, &end, 10);
	if (*end || number < 0 || number > 30) return false;
	result.kind = ARM64RegisterKind::General;
	result.index = static_cast<uint8_t>(number);
	result.width = width;
	return true;
}

}

bool ParseARM64Register(const char* text, ARM64Register& result)
{
	if (ParseNumbered(text, 'x', 64, result) ||
		ParseNumbered(text, 'w', 32, result)) return true;
	if (Equal(text, "sp")) result = {ARM64RegisterKind::StackPointer, 31, 64};
	else if (Equal(text, "wsp"))
		result = {ARM64RegisterKind::StackPointer, 31, 32};
	else if (Equal(text, "xzr")) result = {ARM64RegisterKind::Zero, 31, 64};
	else if (Equal(text, "wzr")) result = {ARM64RegisterKind::Zero, 31, 32};
	else if (Equal(text, "fp")) result = {ARM64RegisterKind::General, 29, 64};
	else if (Equal(text, "lr")) result = {ARM64RegisterKind::General, 30, 64};
	else return false;
	return true;
}

bool ParseARM64Condition(const char* text, ARM64Condition& result)
{
	struct Entry {
		const char* name;
		ARM64Condition condition;
	};
	static const Entry entries[] = {
		{"eq", ARM64Condition::EQ}, {"ne", ARM64Condition::NE},
		{"cs", ARM64Condition::CS}, {"cc", ARM64Condition::CC},
		{"mi", ARM64Condition::MI}, {"pl", ARM64Condition::PL},
		{"vs", ARM64Condition::VS}, {"vc", ARM64Condition::VC},
		{"hi", ARM64Condition::HI}, {"ls", ARM64Condition::LS},
		{"ge", ARM64Condition::GE}, {"lt", ARM64Condition::LT},
		{"gt", ARM64Condition::GT}, {"le", ARM64Condition::LE}
	};
	for (const Entry& entry : entries)
		if (Equal(text, entry.name)) {
			result = entry.condition;
			return true;
		}
	return false;
}

}
