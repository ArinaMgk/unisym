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
#include "../../../inc/c/ustdbool.h"

coe* CoePow(coe* dest, const coe* sors)
{
	if (dest->divr[1] == '0' || sors->divr[1] == '0')
	{
		if (dest->divr[1] != '0')srs(dest->coff, StrHeap(sors->coff));
		return dest;
	}
	if (dest->coff[1] == '0')
	{
		int zo_nan = sors->coff[1] == '0';
		return CoeRst(dest, StrHeap(zo_nan ? coenan.coff : "+0"), StrHeap(zo_nan ? coenan.expo : "+0"), StrHeap(zo_nan ? coenan.divr : "+1"));
	}
	if (sors->coff[1] == '0')
		return CoeRst(dest, StrHeap("+1"), StrHeap("+0"), StrHeap("+1"));// coeone
	if (*sors->coff == '+' && *sors->expo == '+' && !StrCompare(sors->divr, "+1"))
	{
		coe* CrtExp = CoeCpy(sors);
		coe* ret = CoeCpy(dest);// attention! the char* will be free, so use this but "coe ret;" in stack
		CoeSub(CrtExp, &coeone);
		while (CoeCmp(CrtExp, &coezero) > 0)
		{
			size_t retlen = StrLength(ret->coff);
			if (StrLength(dest->coff) + retlen + 4 > malc_limit)CoeDig(ret, malc_limit - 4 - retlen, 2);// Haruno fix RFC04: 4 is random magika number
			CoeMul(ret, dest);
			CoeSub(CrtExp, &coeone);
			// void CoePrint(coe * co); CoePrint(ret); CoePrint(dest);
			// srs(CrtExp, ChrSub(CrtExp, "+1"));
		}
		CoeDel(CrtExp);
		CoeRst(dest, StrHeap(ret->coff), StrHeap(ret->expo), StrHeap(ret->divr));
		CoeDel(ret);
	}
	else
	{
		// Pow(a,b) == Exp( mul( log(a) , b) )
		int t_sign = (*dest->coff == '-');
		ptrdiff_t t_expo = atoins(dest->expo);
		if (!(t_expo & 1)) t_sign = 0;
		dest = CoeLog(dest);
		CoeMul(dest, sors);
		CoeExp(dest);
		*dest->coff = t_sign ? '-' : '+';
	}
	return dest;
}
