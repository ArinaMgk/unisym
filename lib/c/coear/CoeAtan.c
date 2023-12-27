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

// Not recommend
coe* CoeAtan(coe* dest)
{
	if (dest->divr[1] == '0') return dest;
	// Haruno RFR11
	// Considering the Taylor is true for [-1,1].
	// E.g. atan(3.14 = 1+1+1+0.14), 1 for recursion and 0.14 for Taylor.
	// let a =atan A, b=atan B
	// tan(a+b)=(tan a+tan b)/(1-tan a*tan b)
	// tan(a-b)=(tan a-tan b)/(1+tan a*tan b)
	// atan(A)-atan(B)=atan(A-B/(1+A*B))
	// so atan(1+0.14)=atan 0.14+atan(/(1+1.14*0.14))=pi/4 + atan(0.14/2.14) (B=0.14 and B=1) -- [checked]
	// [checked] pi/4 == atan(1.14/0.86)-atan 0.14 == atan 0.14+atan(0.86/1.14)
	int state = 0;
	if (dest->coff[0] == '-')
	{
		dest->coff[0] = '+';
		dest = CoeAtan(dest);
		*dest->coff = (*dest->coff == '-' ? '+' : '-');
	}
	else if ((state = CoeCmp(dest, &coeone)) == 0)
	{
		CoeRst(dest, StrHeap(coepi_quarter.coff), StrHeap(coepi_quarter.expo), StrHeap(coepi_quarter.divr));
		CoeDig(dest, show_precise, 2);
	}
	else if (state == 1)
	{
		// atan(A)=atan( div(A-1,A+1) )+pi/4
		coe* a = CoeCpy(dest); CoeSub(a, &coeone);
		coe* b = CoeCpy(dest); CoeAdd(b, &coeone);
		CoeDiv(a, b);
		CoeAtan(a);
		CoeAdd(a, &coepi_quarter);
		CoeRst(dest, StrHeap(a->coff), StrHeap(a->expo), StrHeap(a->divr));
		CoeDel(a); CoeDel(b);
		// atan(A)=atan(1/(1+A*(A-1)))+atan(A-1)
	}
	else
	{
		// tan(a) = 2*tan(a/2)/(1-tan(a/2)*tan(a/2)) ?
		CoeTaylor(dest, _CDETayFunc_Atn, 0, _DIG_CUT);
	}
	return dest;
}
