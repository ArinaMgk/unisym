// ASCII C99 TAB4 CRLF
// Docutitle: (Module) ASCII Powerful Number of Arinae
// Codifiers: @ArinaMgk(RFZ23); @dosconio: 20240422 ~ <Last-check> 
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
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
#include "../../../inc/c/arith.h"
#include "../../../inc/c/coear.h"
#include "../../../inc/c/uctype.h"
#if !defined(_MCCA)
#include <float.h>

#define isneostr(x) (x) // is not end of string
#define issign(x) ((x)=='+'||(x)=='-')

double atoflt(const char* astr)
{
	if (!astr) return 0.0;//{}
	double res = 0.0;
	double fraction = 1.0;
	int exist_sign = 1;
	int exist_point = 0;
	if (issign(*astr) && *astr++ == '-')
		exist_sign = -1;
	while (isneostr(*astr)) {
		if (ascii_isspace(*astr));
		else if (ascii_isdigit(*astr)) {
			if (exist_point) {
				fraction /= 10.0;
				res += (*astr - '0') * fraction;
			}
			else res = res * 10.0 + (*astr - '0');
		}
		else if (*astr == '.') {
			if (!exist_point) exist_point = 1;
			else return res * exist_sign;
		}
		else return res * exist_sign;
		astr++;
	}
	return res * exist_sign;
}

#define restrict

static bool MatchInsensitive(const char* input, const char* token)
{
	while (*token) {
		if (ascii_tolower(*input++) != ascii_tolower(*token++)) return false;
	}
	return true;
}

static int FloatingDigit(char current, int base)
{
	int digit;
	if (ascii_isdigit(current)) digit = ascii_digtoins(current);
	else if (ascii_isalpha(current)) digit = ascii_tolower(current) - 'a' + 10;
	else return -1;
	return digit < base ? digit : -1;
}

enum { StrTokenExponentLimit = 1000000 };

static void AdjustFloatingExponent(int* exponent, int amount)
{
	if (amount > 0 && *exponent > StrTokenExponentLimit - amount)
		*exponent = StrTokenExponentLimit;
	else if (amount < 0 && *exponent < -StrTokenExponentLimit - amount)
		*exponent = -StrTokenExponentLimit;
	else *exponent += amount;
}

static int ParseFloatingExponent(const char** input)
{
	const char* ptr = *input;
	bool negative = false;
	if (*ptr == '-' || *ptr == '+') {
		negative = *ptr == '-';
		ptr++;
	}
	int exponent = 0;
	while (ascii_isdigit(*ptr)) {
		int digit = ascii_digtoins(*ptr++);
		if (exponent <= (StrTokenExponentLimit - digit) / 10)
			exponent = exponent * 10 + digit;
		else exponent = StrTokenExponentLimit;
	}
	*input = ptr;
	return negative ? -exponent : exponent;
}

static long double ScaleFloating(long double value, int exponent,
	long double factor)
{
	bool negative = exponent < 0;
	unsigned int magnitude = negative ?
		(unsigned int)(-exponent) : (unsigned int)exponent;
	while (magnitude) {
		if (magnitude & 1U)
			value = negative ? value / factor : value * factor;
		magnitude >>= 1;
		if (magnitude) factor *= factor;
	}
	return value;
}

static long double ParseFloating(const char* restrict inp,
	char** restrict endptr)
{
	if (!inp) {
		if (endptr) *endptr = NULL;
		return 0.0L;
	}
	const char* const original = inp;
	const char* ptr = inp;
	while (ascii_isspace(*ptr)) ptr++;
	bool negative = false;
	if (*ptr == '-' || *ptr == '+') {
		negative = *ptr == '-';
		ptr++;
	}
	if (MatchInsensitive(ptr, "inf")) {
		ptr += MatchInsensitive(ptr, "infinity") ? 8 : 3;
		if (endptr) *endptr = (char*)ptr;
		return negative ? -HUGE_VALL : HUGE_VALL;
	}
	if (MatchInsensitive(ptr, "nan")) {
		ptr += 3;
		if (*ptr == '(') {
			const char* payload = ptr + 1;
			const char* close = payload;
			while (ascii_isalnum(*close) || *close == '_') close++;
			if (*close == ')') ptr = close + 1;
		}
		if (endptr) *endptr = (char*)ptr;
		long double value = NAN;
		return negative ? -value : value;
	}

	bool hexadecimal = false;
	if (ptr[0] == '0' && ascii_toupper(ptr[1]) == 'X') {
		const char* first = ptr + 2;
		if (FloatingDigit(*first, 16) >= 0 ||
			(*first == '.' && FloatingDigit(first[1], 16) >= 0)) {
			hexadecimal = true;
			ptr += 2;
		}
	}
	const int base = hexadecimal ? 16 : 10;
	const int digit_exponent = hexadecimal ? 4 : 1;
#ifdef LDBL_DECIMAL_DIG
	const unsigned decimal_digits = LDBL_DECIMAL_DIG + 2;
#else
	const unsigned decimal_digits = 40;
#endif
	const unsigned significant_limit = hexadecimal ?
		(unsigned)((LDBL_MANT_DIG + 3) / 4 + 2) : decimal_digits;
	long double result = 0.0L;
	unsigned significant = 0;
	int exponent = 0;
	bool converted = false;
	bool point = false;
	bool nonzero = false;
	for (;;) {
		int digit = FloatingDigit(*ptr, base);
		if (digit >= 0) {
			converted = true;
			if (point) AdjustFloatingExponent(&exponent, -digit_exponent);
			if (digit || nonzero) {
				nonzero = true;
				if (significant < significant_limit) {
					result = result * (long double)base + digit;
					significant++;
				}
				else AdjustFloatingExponent(&exponent, digit_exponent);
			}
			ptr++;
			continue;
		}
		if (!point && *ptr == '.') {
			point = true;
			ptr++;
			continue;
		}
		break;
	}
	if (!converted) {
		if (endptr) *endptr = (char*)original;
		return 0.0L;
	}
	char exponent_marker = hexadecimal ? 'P' : 'E';
	if (ascii_toupper(*ptr) == exponent_marker) {
		const char* exponent_start = ptr;
		const char* exponent_digits = ptr + 1;
		if (*exponent_digits == '-' || *exponent_digits == '+')
			exponent_digits++;
		if (ascii_isdigit(*exponent_digits)) {
			ptr++;
			AdjustFloatingExponent(&exponent, ParseFloatingExponent(&ptr));
		}
		else ptr = exponent_start;
	}
	if (result != 0.0L)
		result = ScaleFloating(result, exponent,
			hexadecimal ? 2.0L : 10.0L);
	if (endptr) *endptr = (char*)ptr;
	return negative ? -result : result;
}

double StrTokenDouble(const char* restrict inp, char** restrict endptr)
{
	return (double)ParseFloating(inp, endptr);
}
/* 20240822
int main() {
	const char* str = "123.456e-2e";
	char* endptr;
	double value = StrTokenDouble(str, &endptr);
	printf("Value: %f\n", value);
	printf("Endptr: %s\n", endptr);
	return 0;
}*//*
Value: 12345.600000
Endptr: e
*/

long double StrTokenLDouble(const char* restrict inp, char** restrict endptr)
{
	return ParseFloating(inp, endptr);
}

float StrTokenFloat(const char* restrict inp, char** restrict endptr)
{
	return (float)ParseFloating(inp, endptr);
}



double CoeToDouble(const coe* dest)
{
	if (dest->divr[1] == '0')
	{
		return (dest->coff[1] == '0' ? NAN : INFINITY);
	}
	coe* ddd = CoeCpy(dest);
	CoeDivrUnit(ddd, show_precise);
	double ll = 0.0;
	ptrdiff_t CrtPow = atoins(ddd->expo);
	if (CrtPow > DBL_MAX_10_EXP) return INFINITY;
	for (ptrdiff_t i = StrLength(ddd->coff) - 1; i > 0; i--)
	{
		if (CrtPow >= DBL_MIN_10_EXP)
			ll += (ddd->coff[i] - '0') * pow(10.0, (double)CrtPow);
		CrtPow++;
		if (CrtPow > DBL_MAX_10_EXP) break;
	}
	if (*ddd->coff == '-')ll *= -1;
	CoeDel(ddd);
	return ll;
}
#endif
