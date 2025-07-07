/// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ23
// AllAuthor: @ArinaMgk
// ModuTitle: ASCII Powerful Number of Arinae
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

#include "../../../inc/c/coear.h"

coe* CoeFromLocale(const char* str)
{
	const char* ParA = str, * ParB = 0, * ParC = 0;
	size_t LenA = 0, LenB = 0, LenC = 0;
	char c;
	coe* co = 0;
	size_t coflen = 0;
loop: switch (c = *str++)
	{
	case 0:
		// Chk and gen co.
		memalloc(co, sizeof(coe));
		co->symb = 0;
		co->divr = StrHeap("+1");
		if (ParC) LenC = str - ParC - 1;// e2$$
		else if (ParB) LenB = str - ParB - 1;
		else LenA = str - ParA;

		if (ParC && LenC && ParC[LenC - 1] != '+' && ParC[LenC - 1] != '-')
		{
			co->expo = StrHeapN(ParC, LenC);
			if (*co->expo != '+' && *co->expo != '-')
				srs(co->expo, StrHeapAppend(("+"), co->expo));
		}
		else co->expo = StrHeap("+0");
		if (!LenA && !LenB)
		{
			co->coff = StrHeap("+1");
			goto loopend;
		}

		if (!ParB || ParA == ParB || !LenB)// only integer
		{
			co->coff = StrHeapN(ParA, LenA);
			if (*co->coff != '+' && *co->coff != '-')
				srs(co->coff, StrHeapAppend(("+"), co->coff));
			if (!co->coff[1])
				srs(co->coff, StrHeapAppend(co->coff, "1"));
			goto loopend;
		}
		else
		{
			memalloc(co->coff, LenA + LenB + 1);
			if (LenA)
				StrCopyN(co->coff, ParA, LenA);// 0.x case
			StrCopyN(co->coff + LenA, ParB, LenB);
			co->coff[LenA + LenB] = 0;
			char* tmpptr = instoa(LenB);
			srs(co->expo, ChrSub(co->expo, tmpptr));
			memfree(tmpptr);
			if (*co->coff != '+' && *co->coff != '-')
				srs(co->coff, StrHeapAppend(("+"), co->coff));
			if (!co->coff[1])
				srs(co->coff, StrHeapAppend(co->coff, "1"));
			goto loopend;
		}
		break;
	default:
		if (c >= '0' && c <= '9');
		else goto loopend;
		break;
	case '+':
	case '-':
		break;
	case 'e':
		if (ParC) goto loopend;
		ParC = str;
		if (ParA && ParB)//1.2e3
		{
			LenB = str - ParB - 1;
			ParC = str;
		}
		else if (ParA && !ParB)//1e2
		{
			LenA = str - ParA - 1;
			ParB = ParA;
			ParC = str;
		}
		else goto loopend;
		break;
	case '.':
		if (ParB || ParC) goto loopend;
		ParB = str;
		LenA = str - ParA - 1;//1.1
		break;
	} goto loop; 
	loopend:
	if (!co) return co;
	ChrCpz(co->coff);// +002$ 
	ChrCpz(co->expo);
	return co;
}
