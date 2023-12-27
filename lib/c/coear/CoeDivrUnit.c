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

// Is bound to lost precise! @RFW25
// DIVR HAS NOT A REAL SIGN!!!
coe* CoeDivrUnit(coe* obj, size_t kept_precise)
{
	if (obj->divr[1] == '0') return obj;
	if (!StrCompare(obj->divr, "+1"))
		return obj;
	size_t sorlen = 0;
	while (obj->divr[sorlen])sorlen++;
	if (!kept_precise || kept_precise + sorlen + 2 > malc_limit)
		return 0;
	char* ptor = salc(kept_precise + 2);
	*ptor = '+'; MemSet(ptor + 1, '9', kept_precise);// test how long for result.

	char* refer = ChrMul(ptor, obj->divr);
	size_t refer_len = 0;// excluding sign
	for (; refer[refer_len + 1]; refer_len++);
	memfree(refer); memfree(ptor);

	size_t coff_len = 0;// excluding sign
	for (; obj->coff[coff_len + 1]; coff_len++);

	char* quo, * tmppp;
	if (coff_len < refer_len)// compensate 0
	{
		memalloc(tmppp, refer_len + 2);
		quo = StrCopy(tmppp, obj->coff);// trans pointer
		MemSet(quo + 1 + coff_len, '0', refer_len - coff_len);
		quo[refer_len + 1] = 0;
		// borrow === refer_len - coff_len
	}
	else 
	{
		memalloc(tmppp, coff_len + 2);
		quo = StrCopy(tmppp, obj->coff);
	}
	char* rem = malc(sorlen + 2);
	StrCopy(rem, obj->divr);
	ChrDiv(quo, rem);
	if (atoins(rem) * 2 > atoins(obj->divr))
		srs(quo, ChrAdd(quo, "+1"));// bound
	size_t quo_len = 0;// excluding sign
	for (; quo[quo_len + 1]; quo_len++);
	if (quo_len < kept_precise)
		erro("Precise Exception yo CoeDivrUnit()");
	quo[kept_precise + 1] = 0;
	if (quo[kept_precise + 1] >= '5')// round
		srs(quo, ChrAdd(quo, (*quo == '+') ? "+1" : "-1"));
	srs(obj->coff, quo);
	srs(obj->divr, StrHeap("+1"));
	//expo
	char* tmp_expo_inter;
	if (coff_len < refer_len)
	{
		srs(obj->expo, ChrSub(obj->expo, tmp_expo_inter = instoa(((refer_len - coff_len) - (quo_len - kept_precise)))));
	}
	else srs(obj->expo, ChrAdd(obj->expo, tmp_expo_inter = instoa(quo_len - kept_precise)));
	memfree(tmp_expo_inter);
	memfree(rem);
	return obj;
}
