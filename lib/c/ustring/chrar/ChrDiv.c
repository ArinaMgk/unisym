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

// ASCII String Div (without input check)(aflaga.signsym decide I&O but just O)
void ChrDiv(char* a, char* b)
{
	if (!a || !b || !*a || !*b)// {TODO} option to close, for faster speed of arith.
	{
		#ifndef _ARN_FLAG_DISABLE// with malc_states
		///malc_occupy = 0;
		aflaga.overflow = 1;
		aflaga.fail = 1;
		call_state = 0;
		#endif
		return;
	}
	struct { unsigned SignB : 1, SignA : 1; } flag = { 0 };
	int tmp;//
#ifndef _ARN_FLAG_DISABLE
	if (aflaga.signsym)
	{
		if (*a != '+' && *a != '-' || *b != '+' && *b != '-')
		{
			#ifndef _ARN_FLAG_DISABLE
			///malc_occupy = 0;
			aflaga.fail = 1;
			call_state = 2;
			#endif
			return;// BAD INPUT
		}
		int sign_a = (*a == '-') ^ (*b == '-');
		aflaga.sign = sign_a;
		*b++ = *a;
		*a++ = sign_a ? '-' : '+';
	}
#else
	aflaga.sign = 0;
#endif
	char* endofa = a, * endofb = b;// last digits
	for (; '0' <= *endofa && *endofa <= '9'; endofa++); endofa--;
	for (; '0' <= *endofb && *endofb <= '9'; endofb++); endofb--;
	if (endofa < a || endofb < b)
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflaga.fail = 1;
		call_state = 2;
		#endif
		return;
	}
	if ((a < b) && (endofa + 1 >= b - 1) || (a == b) || (a > b) && (endofb + 1 >= a - 1))
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflaga.fail = 1;
		call_state = 2;
		#endif
		return;// INPUT CROSS
	}
	if (*b == '0')
	{
		#ifndef _ARN_FLAG_DISABLE
		///malc_occupy = 0;
		aflaga.fail = 1;
		call_state = 2;
		#endif
		aflaga.overflow = 1;
		return;// EXCEPTION
	}
	if (*a == '0' || (endofb - b > endofa - a) || (endofb - b == endofa - a && (StrCompare(a, b) < 0)))
	{
		for (register int _c = 0; _c < endofa - a + 1; _c++) b[_c] = a[_c];
		b[endofa - a + 1] = 0;
		*a++ = '0'; *a = 0;
		#ifndef _ARN_FLAG_DISABLE
		aflaga.fail = 0;
		aflaga.overflow = 0;
		#endif
		return;
	}
	size_t siz = endofa - a + 2;// sign and ter-0
	#ifndef _ARN_FLAG_DISABLE
	if (siz > malc_limit)
	{
		///malc_occupy = 0;
		aflaga.fail = 1;
		call_state = 1;
		return;// LACK MEMORY
	}
	#endif
	char* buf_sub = salc(siz);
	char* const buf_sub_fix = buf_sub;
	size_t buf_rem = siz - 1;
	const size_t div_size = endofb - b + 1;
	for (register int _c = 0; _c < endofa - a + 1; _c++)
	{
		buf_sub[_c] = a[_c];
		a[_c] = '0';
	}
	while (buf_rem > div_size)
	{
		tmp = 0;
		if (StrCompareN(buf_sub, b, div_size) >= 0)
		{
			do
			{
				tmp++;
				for (register unsigned int _c = 0; _c < div_size; _c++)
					DigDec(b[div_size - 1 - _c],
						(char*)((size_t)buf_sub + div_size - 1 - _c));
			} while (StrCompareN(buf_sub, b, div_size) >= 0);
			*(endofa - buf_rem + div_size) = '0' + tmp;
		}
		else
		{
			do
			{
				tmp++;
				for (register unsigned int _c = 0; _c < div_size; _c++)
					DigDec(b[div_size - 1 - _c],
						(char*)((size_t)buf_sub + div_size - _c));
			} while (StrCompareN(buf_sub, b, div_size) < 0 && *buf_sub != '0' ||
				(StrCompareN(buf_sub + 1, b, div_size) >= 0 && *buf_sub == '0'));
			*(endofa - buf_rem + div_size + 1) = '0' + tmp;
		}
		for (; *buf_sub == '0' && buf_sub[1] != 0; buf_rem--, buf_sub++);
	}
	if (buf_rem == div_size)
	{
		if ((tmp = StrCompareN(buf_sub, b, buf_rem)) < 0)
		{
			for (register unsigned int _c = 0; _c < buf_rem; _c++) b[_c] = buf_sub[_c]; b[buf_rem] = 0;
		}
		else if (tmp == 0)
		{
			b[0] = '0'; b[1] = 0; DigInc('1', endofa);
		}
		else
		{
			tmp = 0;
			do
			{
				tmp++;
				for (register size_t _c = 0; _c < buf_rem; _c++)
					DigDec(b[div_size - 1 - _c], (char*)((size_t)buf_sub + buf_rem - 1 - _c));
			} while (StrCompareN(buf_sub, b, buf_rem) >= 0);
			DigInc('0' + tmp, endofa);
			for (; *buf_sub == '0'; buf_rem--, buf_sub++);
			for (size_t _c = 0; _c < buf_rem; _c++) b[_c] = buf_sub[_c]; /* b[buf_rem] = 0; */
			if(buf_rem) b[buf_rem] = 0;// fix @RFW16
			else
			{
				*b = '0';
				b[1] = 0;
			}
		}
	}
	else
	{
		for (register unsigned int _c = 0; _c < buf_rem; _c++)
			b[_c] = buf_sub[_c]; b[buf_rem] = 0;
	}
	memfree(buf_sub_fix);
	ChrCpz(a);
	aflaga.overflow = 0;
	aflaga.fail = 0;
	return;
}
