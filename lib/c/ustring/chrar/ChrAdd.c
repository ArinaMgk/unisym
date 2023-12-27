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

// ASCII String Add (without input check)
char* ChrAdd(const char* a, const char* b)
{
	// may xchg to make length a >= b
	if (!a || !b || !*a || !*b)
	{
		#ifndef _ARN_FLAG_DISABLE// with malc_states
		///malc_occupy = 0;
		aflaga.overflow = 1;
		aflaga.fail = 1;
		call_state = 0;
		#endif
		return 0;
	}
	struct { unsigned carry : 1, SignB : 1, SignA : 1; } flag = { 0 };
	if (*a == '+') { a++; }
	if (*a == '-') { a++;flag.SignA = 1; }
	if (*b == '+') { b++; }
	if (*b == '-') { b++;flag.SignB = 1; }
	if (flag.SignA ^ flag.SignB)
	{
		// the control of aflag is given to ChrSub(). 
		return (flag.SignA ? ChrSub(b, a) : ChrSub(a, b));
	}
	const char* endofa = a, * endofb = b;// last digit
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
	if (endofa - a < endofb - b) // xchg
	{
		xchgptr(a, b);
		xchgptr(endofa, endofb);
		xchg(flag.SignA, flag.SignB);
	}
	size_t siz = endofa - a + 4;// Magic 4: Potential Symbol, \0, LengthDif:1, CarryDigit
	#ifndef _ARN_FLAG_DISABLE
	if (siz > malc_limit)
	{
		///malc_occupy = 0;
		aflaga.fail = 1;
		call_state = 1;
		return 0;
	}
	#endif
	char* q = malc(siz);
#ifndef _ARN_FLAG_DISABLE// or execuate once
	aflaga.sign = flag.SignA;
	if (aflaga.signsym)
	{
		q[siz - 1] = 0;
		*q = flag.SignA ? '-' : '+';
	} else q[siz - 2] = 0;
#else
	q[siz - 1] = 0;
	*q = flag.SignA ? '-' : '+';
#endif
	size_t loop_time = siz - 3;// detail to see old version
	////for (size_t i = 0; i < siz - 3; i++)
	if(loop_time) do
	{
		unsigned char regichar = *(endofa--) - 0x30;
		if (endofb >= b) regichar += *(endofb--) - 0x30;
		regichar += flag.carry;
		if (flag.carry = (regichar > 9)) regichar -= 10;
#ifndef _ARN_FLAG_DISABLE
		q[loop_time-- + aflaga.signsym] = regichar + 0x30;
#else
		q[loop_time-- + 1] = regichar + 0x30;
#endif
	} while (loop_time);
#ifndef _ARN_FLAG_DISABLE
	q[aflaga.signsym ? 1 : 0] = flag.carry + 0x30;
	ChrCpz(q);
	aflaga.overflow = 0;
	///malc_occupy = siz;
	aflaga.fail = 0;
#else
	q[1] = flag.carry + 0x30;
	ChrCpz(q);
#endif
	return q;
}
