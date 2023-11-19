// ASCII TAB4 C99 ArnAssume
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>// na isXType()
#include "../../inc/c/alice.h"
#include "../../inc/c/aldbg.h"
#define _LIB_STRING_BUFFER// {TODO}
#define _LIB_STRING_HEAP// {TODO}
#include "../../inc/c/ustring.h"

#define erro(x) ((void)(x))// CHEAT

struct ArinaeFlag arna_eflag = {0};

//---- ---- ---- ---- string ---- ---- ---- ----
#if 1// convenient for reading codes
void StrFilterOut(char* p, char c)
{
	char* q = p, chr;
	for (; (chr = *p) != 0; p++)
		if (chr != c)
			*q++ = chr;
	*q = 0;
}

void StrFilter(char* p, enum TokType tt)
{
	char* q = p;
	char c;
	int (*jdg)(int);
	switch (tt)
	{
	case tok_space: jdg = isspace; break;
	case tok_number: jdg = isdigit; break;
	// ...
	default: return;
	}
	for (; (c = *p) != 0; p++)
		if (jdg(c))
			*q++ = c;
	*q = 0;
}

void StrFilterString(char* p, const char* needs)
{
	char* q = p;
	char c;
	for (; (c = *p) != 0; p++)
		if (StrIndexChar(needs, c))
			*q++ = c;
	*q = 0;
}

void StrFilterOutString(char* p, const char* neednot)
{
	char* q = p;
	char c;
	for (; (c = *p) != 0; p++)
		if (!StrIndexChar(neednot, c))
			*q++ = c;
	*q = 0;
}

size_t StrDeprefixSpaces(char* str)// Base on ChrCpz RFV30
{
	size_t siz = StrLength(str);
	ptrdiff_t num = 0;
	char c;
	while ((c = str[num]) && isspace(c)) num++;
	if (!num) return 0;
	else MemRelative(str + num, siz - num + 1, -num);
	return num;
}

size_t StrDesuffixSpaces(char* str)// Base on ChrCtz RFV30
{
	size_t coflen = 0,
		num = 0;
	while (str[coflen]) coflen++;
	while (isspace(str[coflen - num - 1])) num++;
	str[coflen - num] = 0;
	return num;
}

#endif

// // ---- ---- ---- sorting ---- ---- ---- // //

void StrSortBubble(char* str, int order)
{
	size_t len = StrLength(str);
	char tmp;
	for (size_t i = 0; i < len - 1; i++)
	{
		for (size_t j = i + 1; j < len; j++)
		{
			if (order ? str[i] < str[j] : str[i] > str[j])
			{
				tmp = str[i];
				str[i] = str[j];
				str[j] = tmp;
			}
		}
	}
}

// // ---- ---- ----   ---- ---- ---- // //

size_t size_dec = 0; void size_dec_get()
{
	register unsigned int i = 0;
	register size_t r = (size_t)~0;
	while (r)
		i++, r /= 10;
	//x64 for 20, and x86 for 10
	size_dec = i;
}

char* instob(ptrdiff_t num, char* buf)
{
	#ifdef _ARN_FLAG_ENABLE
	size_t numlen = aflag.Signed + 1;
	#else
	size_t numlen = 2;
	#endif
	size_t numslv = num > 0 ? num : -num;
	AddDecimalDigitsLen(numlen, numslv);
	#ifdef _ARN_FLAG_ENABLE
	if (malc_limit < numlen || malc_limit < (unsigned)3 - !arna_eflag.Signed || !buf)
	{
		///malc_occupy = 0;
		arna_eflag.Failure = 1;
		call_state = 1;
		return 0;
	}
	#endif
	//
	char* p = buf + numlen - 2;
	#ifdef _ARN_FLAG_ENABLE
	if (aflag.Signed)
	#endif
		*buf = num < 0 ? '-' : '+';
	buf[numlen - 1] = 0;
	numslv = num > 0 ? num : -num;
	do
	{
		*p-- = (numslv % 10) + '0';
		numslv /= 10;
	} while (numslv);
	#ifdef _ARN_FLAG_ENABLE
	aflag.PrecLoss = 0;
	aflag.Failure = 0;
	aflag.Sign = num < 0;
	///malc_occupy = numlen;
	#endif
	return buf;
}

ptrdiff_t atoins(const char* str)
{
	int sign = 0; int Signed = 0;
	///if (!str || !*str)return 0;
	if (*str == '-') sign++, str++, Signed++;
	else if (*str == '+') str++, Signed++;
	if (!size_dec) size_dec_get();
	const char* ptr = str;
	ptrdiff_t inst = 0;
	while (*ptr && *ptr <= '9' && *ptr >= '0' && (size_t)(ptr - str + !Signed) <= size_dec)
	{
		inst *= 10;
		inst += *ptr - '0';
		ptr++;
	}
	///if (*ptr <= '9' && *ptr >= '0') ...
	return sign ? -inst : inst;
}

//---- ---- ---- ---- ChrAr ---- ---- ---- ----

// Clear prefix zeros of hexa. E.g. "00012500" >>> "12500", "-00" >>> "-0"
// No need to make a specific buffer-version
size_t ChrCpz(char* str)
{
	// assume str:not-null
	if (*str == '+' || *str == '-') str++;
	size_t siz = StrLength(str);
	ptrdiff_t num = 0;
	char c;
	while ((c = str[num]) && c == '0') num++;
	if (!num) return 0;
	if (c == 0 && str[num - 1] == '0')
	{
		str[1] = 0;
		return num - 1;
	}
	else MemRelative(str + num, siz - num + 1, -num);// 000905_ : str=0 num=3 siz=6
	return num;
}

size_t ChrCtz(char* str)
{
	size_t coflen = 0,
		num = 0;// numof 0
	while (str[coflen]) coflen++;
	while (str[coflen - num - 1] == '0') num++;
	if (str[coflen - num - 1] == '-' || str[coflen - num - 1] == '+') num--;
	str[coflen - num] = 0;
	return num;
}

int ChrCmp(const char* a, const char* b)
{
	int sga = 0, sgb = 0;
	const char* endofa = a, * endofb = b;
	int greater = 1;
	if (!StrCompare(a, b)) return 0;
	for (; *endofa; endofa++); endofa--;
	for (; *endofb; endofb++); endofb--;
	if (*a == '+')a++;
	if (*b == '+')b++;
	if (*a == '-') { a++; sga = 1; }
	if (*b == '-') { b++, sgb = 1; }
	if (*a == '0' && *b == '0') return 0;// more effective
	if (endofa - a == endofb - b) greater = (StrCompare(a, b)) > 0;
	else greater = endofa - a > endofb - b;
	if (sga ^ sgb) return sga ? -1 : 1;
	return ((!sga) ^ (!greater)) ? 1 : -1;
}

void DigInc(int ascii, char* posi)
{
	if ((ascii = *posi += ascii -= '0') <= '9') return;
	do *posi-- = ascii - 10; while ((ascii = *posi + 1) > '9'); (*posi)++;
}

void DigDec(int ascii, char* posi)
{
	if ((ascii = *posi -= ascii -= '0') >= '0') return;
	do *posi-- = ascii + 10; while ((ascii = *posi - 1) < '0'); (*posi)--;
}

//---- ---- ---- ---- HISTORY ---- ---- ---- ----
// ...
// RFR18 Append Dnode into ustring
// RFT02 Append ChrAr into ustring
// ...
// RFB19 Fix some mistakes
// RFX07 Move out STRPOOL into HEAP

