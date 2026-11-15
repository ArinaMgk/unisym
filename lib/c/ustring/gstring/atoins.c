// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant 
// LastCheck: RFZ22
// AllAuthor: @dosconio
// ModuTitle: Operations for ASCIZ Character-based String
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../../../../inc/c/ustring.h"
#include "../../../../inc/c/uctype.h"

static int sign = 0;// is_negative, a shared object

// decimal series

static void setsym(const char**const inp) {
	sign = 0;
	if (**inp == '-') sign = !sign, (*inp)++;
	else if (**inp == '+') (*inp)++;
}

stduint atohex(const char* str) {
	stduint ret = 0;
	char ch;
	while ((ch = *str++)) {
		if (ch == '0') {
			ret <<= 4;
			continue;
		}
		ch = _MAP_ALNUM_DIGIT(ch);
		if (ch) ret <<= 4, ret += ch;
		else break;
	}
	return ret;
}

ptrdiff_t atoins(const char* str) // hosted-dep
{
	if (!str || !*str) return 0;
	setsym(&str);
	ptrdiff_t inst = 0;
	for0 (i, _size_decimal_get()) if (ascii_isdigit(str[i]))
	{
		inst *= 10;
		inst += (ptrdiff_t)ascii_digtoins(str[i]);
	} else break;
	return sign ? -inst : inst;
}
int _atoidefa(const char* inp) // hosted-dep
{
	if (!inp || !*inp) return 0;
	setsym(&inp);
	int inst = 0;
	for0(i, lookupDecimalDigits(sizeof(int))) if (ascii_isdigit(inp[i]))
	{
		inst *= 10;
		inst += (int)ascii_digtoins(inp[i]);
	} else break;
	return sign ? -inst : inst;
}

long int atolong(const char* inp) // hosted-dep
{
	if (!inp || !*inp) return 0;
	setsym(&inp);
	int inst = 0;
	for0(i, lookupDecimalDigits(sizeof(long))) if (ascii_isdigit(inp[i]))
	{
		inst *= 10;
		inst += (long)ascii_digtoins(inp[i]);
	} else break;
	return sign ? -inst : inst;
}
#ifdef _BIT_SUPPORT_64
long long int atollong(const char* inp) // sint64
{
	if (!inp || !*inp) return 0;
	setsym(&inp);
	long long int inst = 0;
	for0(i, lookupDecimalDigits(sizeof(long long))) if (ascii_isdigit(inp[i]))
	{
		inst *= 10;
		inst += (long long)ascii_digtoins(inp[i]);
	} else break;
	return sign ? -inst : inst;
}
#endif

#define restrict //{TEMP}

static int StrTokenDigit(char current)
{
	if (ascii_isdigit(current)) return ascii_digtoins(current);
	if (ascii_isalpha(current)) return ascii_tolower(current) - 'a' + 10;
	return -1;
}

static bool StrTokenMagnitude(const char* inp, char** endptr, int base,
	unsigned long long positive_limit, unsigned long long negative_limit,
	unsigned long long* magnitude, bool* negative, bool* overflow)
{
	if (!inp) {
		if (endptr) *endptr = NULL;
		return false;
	}
	const char* const original = inp;
	const char* ptr = inp;
	while (ascii_isspace(*ptr)) ptr++;
	*negative = false;
	if (*ptr == '-' || *ptr == '+') {
		*negative = *ptr == '-';
		ptr++;
	}
	if (base && (base < 2 || base > 36)) {
		if (endptr) *endptr = (char*)original;
		return false;
	}
	if ((!base || base == 16) && ptr[0] == '0' &&
		ascii_toupper(ptr[1]) == 'X') {
		int digit = StrTokenDigit(ptr[2]);
		if (digit >= 0 && digit < 16) {
			base = 16;
			ptr += 2;
		}
	}
	if (!base) base = *ptr == '0' ? 8 : 10;
	unsigned long long limit = *negative ? negative_limit : positive_limit;
	unsigned long long result = 0;
	bool converted = false;
	*overflow = false;
	for (;;) {
		int digit = StrTokenDigit(*ptr);
		if (digit < 0 || digit >= base) break;
		converted = true;
		if (result > (limit - (unsigned)digit) / (unsigned)base)
			*overflow = true;
		else if (!*overflow)
			result = result * (unsigned)base + (unsigned)digit;
		ptr++;
	}
	if (!converted) {
		if (endptr) *endptr = (char*)original;
		return false;
	}
	if (endptr) *endptr = (char*)ptr;
	*magnitude = *overflow ? limit : result;
	return true;
}

long int StrTokenLong(const char* restrict inp, char** restrict endptr, int base) // hosted-dep
{
	unsigned long long result;
	bool negative, overflow;
	unsigned long long negative_limit =
		(unsigned long long)(-(LONG_MIN + 1L)) + 1ULL;
	if (!StrTokenMagnitude(inp, endptr, base, (unsigned long long)LONG_MAX,
		negative_limit, &result, &negative, &overflow)) return 0;
	if (overflow) {
		//{TODO} errno = ERANGE;
		return negative ? LONG_MIN : LONG_MAX;
	}
	if (!negative) return (long)result;
	if (result == negative_limit) return LONG_MIN;
	return -(long int)result;
}/*
int main() {
	const char* str = "12345.";
	char* endptr;
	long int result = StrTokenLong(str, &endptr, 10);
	printf("Converted value: %ld\n", result);
	printf("End pointer: %s\n", endptr);
}
*//*
Converted value: 12345
End pointer: .
*/

unsigned long int StrTokenULong(const char* restrict inp, char** restrict endptr, int base)
{
	unsigned long long result;
	bool negative, overflow;
	if (!StrTokenMagnitude(inp, endptr, base, (unsigned long long)ULONG_MAX,
		(unsigned long long)ULONG_MAX, &result, &negative, &overflow)) return 0;
	if (overflow) {
		//{TODO} errno = ERANGE;
		return ULONG_MAX;
	}
	unsigned long value = (unsigned long)result;
	return negative ? 0UL - value : value;
}
#ifdef _BIT_SUPPORT_64
long long int StrTokenLLong(const char* restrict inp, char** restrict endptr, int base)
{
	unsigned long long result;
	bool negative, overflow;
	unsigned long long negative_limit =
		(unsigned long long)(-(LLONG_MIN + 1LL)) + 1ULL;
	if (!StrTokenMagnitude(inp, endptr, base,
		(unsigned long long)LLONG_MAX, negative_limit,
		&result, &negative, &overflow)) return 0;
	if (overflow) {
		//{TODO} errno = ERANGE;
		return negative ? LLONG_MIN : LLONG_MAX;
	}
	if (!negative) return (long long)result;
	if (result == negative_limit) return LLONG_MIN;
	return -(long long)result;
}

unsigned long long int StrTokenULLong(const char* restrict inp, char** restrict endptr, int base)
{
	unsigned long long result;
	bool negative, overflow;
	if (!StrTokenMagnitude(inp, endptr, base, ULLONG_MAX, ULLONG_MAX,
		&result, &negative, &overflow)) return 0;
	if (overflow) {
		//{TODO} errno = ERANGE;
		return ULLONG_MAX;
	}
	return negative ? 0ULL - result : result;
}
#endif

// atohex(inp 0~f)
// atobin(inp 0/1)

