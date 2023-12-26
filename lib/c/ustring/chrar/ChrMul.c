// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ22
// AllAuthor: @dosconio
// ModuTitle: Operations for ChrAr
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

#include "../../../../inc/c/aldbg.h"
#include "../../../../inc/c/ustring.h"

// ASCII String Mul (without input check)
char* ChrMul(const char* a, const char* b)
{
	// may xchg to make length a >= b
	if (!a || !b || !*a || !*b)
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflaga.overflow = 1;
		aflaga.fail = 1;
		call_state = 0;
		#endif
		return 0;
	}
	struct { unsigned carry_int : 8, SignB : 1, SignA : 1; } flag = { 0 };
	char* q = 0, * p = 0;
	size_t siz;
	if (*a == '+') { a++; }
	if (*b == '+') { b++; }
	if (*a == '-') { a++; flag.SignA = 1; }
	if (*b == '-') { b++, flag.SignB = 1; }
	const char* endofa = a, * endofb = b;// last digits
	for (; '0' <= *endofa && *endofa <= '9'; endofa++); endofa--;
	for (; '0' <= *endofb && *endofb <= '9'; endofb++); endofb--;
	if (endofa < a || endofb < b)
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflaga.fail = 1;
		call_state = 2;
		#endif
		return 0;
	}
	if (endofa - a < endofb - b || (endofa - a == endofb - b) && (StrCompare(a, b) < 0))
	{
		xchgptr(a, b);
		xchgptr(endofa, endofb);
		xchg(flag.SignA, flag.SignB);
	}
	#ifndef _ARN_FLAG_DISABLE
	if ((siz = (endofa - a + 1 + endofb - b + 1 + 1 + aflaga.signsym)) > malc_limit)// Termino-Null and Sign, 9*9=81
	{
		///malc_occupy = 0;
		aflaga.fail = 1;
		call_state = 1;
		//{TODO} convert to precise-cut
		return 0;
	}
	#else
	siz = (endofa - a + 1 + endofb - b + 1 + 1 + 1);
	#endif
	q = salc(siz);
	MemSet(q, '0', siz - 1);
#ifndef _ARN_FLAG_DISABLE
	aflaga.sign = (flag.SignA ^ flag.SignB);
	if (aflaga.signsym)
#endif
	{
		*q = (flag.SignA ^ flag.SignB) ? '-' : '+';
	}
	for (int i = 0; i < endofb - b + 1; i++)
	{
		p = q + siz - 2 - i;
		flag.carry_int = 0;
		for (int j = 0; j < endofa - a + 1; j++)
		{
			int tmp = (*(endofa - j) - '0') * (*(endofb - i) - '0') + flag.carry_int;
			if (tmp > 9)
				tmp -= 10 * (flag.carry_int = tmp / 10);
			else flag.carry_int = 0;
			DigInc(tmp + '0', p--);
		}
		if (flag.carry_int) DigInc(flag.carry_int + '0', p--);
	}
	ChrCpz(q);
	#ifndef _ARN_FLAG_DISABLE
	aflaga.overflow = 0;
	///malc_occupy = siz;
	aflaga.fail = 0;
	#endif
	return q;
}
