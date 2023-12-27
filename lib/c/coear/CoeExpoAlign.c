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

int CoeExpoAlign(coe* o1, coe* o2)
{
	if (o1->divr[1] == '0' || o2->divr[1] == '0') return 0;
	int state = ChrCmp(o1->expo, o2->expo);
	if (state == 0) return 1;
	char* limit = instoa(malc_limit);
	if (state < 0) xchg(*(size_t*)&o1, *(size_t*)&o2);
	// now expo: o1 > o2
	char* expdif = ChrSub(o1->expo, o2->expo);
	if (ChrCmp(expdif, limit) >= 0 || *expdif != '+')
	{
		memfree(limit); memfree(expdif);
		erro("CoeExpoAlign-OverLimit or -SystematicError.");
		return 0;
	}
	size_t expdif_num = atoins(expdif);
	srs(o1->expo, StrHeap(o2->expo));
	srs(o1->coff, StrHeapAppendChars(o1->coff, '0', expdif_num));
	memfree(limit); memfree(expdif);
	return 1;
}
