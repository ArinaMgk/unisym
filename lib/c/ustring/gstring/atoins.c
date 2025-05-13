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
	while (ch = *str++) {
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

long int StrTokenLong(const char* restrict inp, char** restrict endptr, int base) // hosted-dep
{
	long int result = 0;
	const char* ptr = inp;
	if (!inp) {
		asserv(endptr)[0] = NULL;
		return 0;
	}
	while (*ptr == ' ' || *ptr == '\t') ptr++;
	setsym(&inp);
	if (!base) {
		if (*ptr == '0') {
			if (ascii_toupper(ptr[1]) == 'X') {
				base = 16;
				ptr += 2;
			}
			else base = 8;
		}
		else base = 10;
	}
	else if (base < 2 || base > 36) {
		asserv(endptr)[0] = (char*)inp;
		return 0;
	}
	while (ascii_isalnum(*ptr)) {
		int digit = _MAP_ALNUM_DIGIT(*ptr);
		if (!digit || digit >= base) break;
		if (result > (LONG_MAX - digit) / base) {
			//{TODO} errno = ERANGE;
			return sign >= 0 ? LONG_MAX : LONG_MIN;
		}
		result = result * base + digit;
		ptr++;
	}
	if (endptr != NULL) {
		*endptr = (char*)ptr;
	}
	return sign * result;
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
	unsigned long int result = 0;
	const char* ptr = inp;
	if (!inp) {
		asserv(endptr)[0] = NULL;
		return 0;
	}
	while (*ptr == ' ' || *ptr == '\t') ptr++;
	if (!base) {
		if (*ptr == '0') {
			if (ascii_toupper(ptr[1]) == 'X') {
				base = 16;
				ptr += 2;
			}
			else base = 8;
		}
		else base = 10;
	}
	else if (base < 2 || base > 36) {
		asserv(endptr)[0] = (char*)inp;
		return 0;
	}
	while (ascii_isalnum(*ptr)) {
		int digit = _MAP_ALNUM_DIGIT(*ptr);
		if (!digit || digit >= base) break;
		if (result > (ULONG_MAX - digit) / base) {
			//{TODO} errno = ERANGE;
			return ULONG_MAX;
		}
		result = result * base + digit;
		ptr++;
	}
	if (endptr != NULL) {
		*endptr = (char*)ptr;
	}
	return result;
}
#ifdef _BIT_SUPPORT_64
long long int StrTokenLLong(const char* restrict inp, char** restrict endptr, int base)
{
	long long int result = 0;
	const char* ptr = inp;
	if (!inp) {
		asserv(endptr)[0] = NULL;
		return 0;
	}
	while (*ptr == ' ' || *ptr == '\t') ptr++;
	setsym(&inp);
	if (!base) {
		if (*ptr == '0') {
			if (ascii_toupper(ptr[1]) == 'X') {
				base = 16;
				ptr += 2;
			}
			else base = 8;
		}
		else base = 10;
	}
	else if (base < 2 || base > 36) {
		asserv(endptr)[0] = (char*)inp;
		return 0;
	}
	while (ascii_isalnum(*ptr)) {
		int digit = _MAP_ALNUM_DIGIT(*ptr);
		if (!digit || digit >= base) break;
		if (result > (LLONG_MAX - digit) / base) {
			//{TODO} errno = ERANGE;
			return sign >= 0 ? LLONG_MAX : LONG_MIN;
		}
		result = result * base + digit;
		ptr++;
	}
	if (endptr != NULL) {
		*endptr = (char*)ptr;
	}
	return sign * result;
}

unsigned long long int StrTokenULLong(const char* restrict inp, char** restrict endptr, int base)
{
	unsigned long long int result = 0;
	const char* ptr = inp;
	if (!inp) {
		asserv(endptr)[0] = NULL;
		return 0;
	}
	while (*ptr == ' ' || *ptr == '\t') ptr++;
	if (!base) {
		if (*ptr == '0') {
			if (ascii_toupper(ptr[1]) == 'X') {
				base = 16;
				ptr += 2;
			}
			else base = 8;
		}
		else base = 10;
	}
	else if (base < 2 || base > 36) {
		asserv(endptr)[0] = (char*)inp;
		return 0;
	}
	while (ascii_isalnum(*ptr)) {
		int digit = _MAP_ALNUM_DIGIT(*ptr);
		if (!digit || digit >= base) break;
		if (result > (ULLONG_MAX - digit) / base) {
			//{TODO} errno = ERANGE;
			return ULLONG_MAX;
		}
		result = result * base + digit;
		ptr++;
	}
	if (endptr != NULL) {
		*endptr = (char*)ptr;
	}
	return result;
}
#endif

// atohex(inp 0~f)
// atobin(inp 0/1)

