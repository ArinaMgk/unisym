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

static int sign = 0;// shared object

// decimal series

static void setsym(const char**const inp) {
	sign = 0;
	if (**inp == '-') sign++, (*inp)++;
	else if (**inp == '+') (*inp)++;
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

#define restrict //{TEMP}

//{TODO} _MAP_ALNUM_DIGIT
/*
long int StrToken_long(const char* restrict inp, char** restrict endptr, int base) // hosted-dep
{
	long int result = 0;
	sign = 1;
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
		asserv(endptr)[0] = inp;
		return 0;
	}
	while (ascii_isalnum(*ptr)) {
		int digit = 0;
		if (ascii_isdigit(*ptr)) {
			digit = *ptr - '0';
		}
		else if (*ptr >= 'a' && *ptr <= 'z') {
			digit = *ptr - 'a' + 10;
		}
		else if (*ptr >= 'A' && *ptr <= 'Z') {
			digit = *ptr - 'A' + 10;
		}
		if (digit >= base) {
			break;
		}
		if (result > (LONG_MAX - digit) / base) {
			// 超出范围
			errno = ERANGE;
			return sign >= 0 ? LONG_MAX : LONG_MIN;
		}
		result = result * base + digit;
		ptr++;
	}
	if (endptr != NULL) {
		*endptr = (char*)ptr;
	}
	return sign * result;
}*/  /*
int main() {
	const char* str = "12345.";
	char* endptr;
	long int result = my_strtol(str, &endptr, 10);
	printf("Converted value: %ld\n", result);
	printf("End pointer: %s\n", endptr);

}
*//*
Converted value: 12345
End pointer: .
*/

// atohex(inp 0~f)
// atobin(inp 0/1)

