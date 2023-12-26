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

coe* CoeDivrAlign(coe* o1, coe* o2)
{
	if (!o1 && o1->divr[1]=='0') return 0;
	if (!o2)// Reduction
	{
		if (o1->coff[1] == '0')
		{
			if (o1->divr[1] != '0')
				srs(o1->divr, StrHeap("+1"));
			return o1;
		}
		char* GCD = ChrComDiv(o1->coff, o1->divr);
		char* GCDCpy = StrHeap(GCD);
		if (StrCompare(GCD + 1, "1"))
		{
			ChrDiv(o1->coff, GCD);
			ChrDiv(o1->divr, GCDCpy);
		}
		memfree(GCD);
		memfree(GCDCpy);
		// then we check if there are +2 and +5 element
		while (StrCompare((GCD = ChrComDiv(o1->divr, "+2")) + 1, "1"))
		{
			char* tmpptr;
			ChrDiv(o1->divr, tmpptr = StrHeap("+2"));
			memfree(tmpptr);
			srs(o1->expo, ChrSub(o1->expo, "+1"));

			tmpptr = StrHeapAppend(o1->coff, "0");
			memfree(o1->coff);
			o1->coff = tmpptr;

			ChrDiv(o1->coff, tmpptr = StrHeap("+2"));
			memfree(GCD); memfree(tmpptr);
		}
		memfree(GCD);
		while (StrCompare((GCD = ChrComDiv(o1->divr, "+5")) + 1, "1"))
		{
			char* tmpptr;
			ChrDiv(o1->divr, tmpptr = StrHeap("+5"));
			memfree(tmpptr);
			srs(o1->expo, ChrSub(o1->expo, "+1"));

			tmpptr = StrHeapAppend(o1->coff, "0");
			memfree(o1->coff);
			o1->coff = tmpptr;

			ChrDiv(o1->coff, tmpptr = StrHeap("+5"));
			memfree(GCD); memfree(tmpptr);
		}
		memfree(GCD);
		return o1;
	}
	// Align expo
	if (o2->divr[1] == '0') return 0;
	CoeExpoAlign(o1, o2);
	char* GCM = ChrComMul(o1->divr, o2->divr);
	char* GCMCpy = StrHeap(GCM);
	char* ori1 = StrHeap(o1->divr),
		* ori2 = StrHeap(o2->divr);
	srs(o1->divr, StrHeap(GCM));
	srs(o2->divr, StrHeap(GCMCpy));
	ChrDiv(GCM, ori1); srs(o1->coff, ChrMul(o1->coff, GCM));
	ChrDiv(GCMCpy, ori2); srs(o2->coff, ChrMul(o2->coff, GCMCpy));
	memfree(GCM); memfree(GCMCpy);
	memfree(ori1); memfree(ori2);
	return o1;
}
