// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ22
// AllAuthor: @dosconio
// ModuTitle: Operations for ASCIZ Character-based String Specially output heap
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


#include "../../../../inc/c/ustring.h"
#include "../../../../inc/c/dnode.h"

// From left, one by one, return all in heap! must free() by yourself. Inputs won't be free!
// RFV07 rename from "StrReplaceHeap"
char* StrReplace(const char* dest, const char* subfirstrom, const char* subto, size_t* times)
{
	struct _aflag_t af = aflaga;
	size_t time_limit = times ? *times : ~_IMM0;
	if (times) *times = 0;
	if (!dest || !subfirstrom || !subto || !malc_limit)
		return zalc(1);
	if (!*dest || !*subfirstrom) return StrHeap(dest);
	dchain_t* dc = zalcof(dchain_t);
	//
	char* ret;
	DchainInit(dc);
	size_t sz_subto = 0, nums = 0, sz_subfirstrom = 0, sz_len = 0;
	ptrdiff_t chars_add = 0;
	for (; subto[sz_subto]; sz_subto++);
	for (; subfirstrom[sz_subfirstrom]; sz_subfirstrom++);
	for (; dest[sz_len]; sz_len++);
	chars_add = (ptrdiff_t)sz_subto - (ptrdiff_t)sz_subfirstrom;
	const char* p = dest;
	while (nums < time_limit)
	{
		p = StrIndexString(p, subfirstrom);
		if (!p) break;
		Dnode* dn = DchainAppend(dc, (pureptr_t)p, false, NULL);
		p += dn->lens = sz_subfirstrom;
		nums++;
	}
	if (nums)
	{
		if (times) *times = nums;
		ret = salc((ptrdiff_t)sz_len + (ptrdiff_t)chars_add * nums + (ptrdiff_t)1);
		p = dest; char* q = ret;
		Dnode* dn = dc->root_node;
	loop:
		for (; p < dn->addr; p++) *q++ = *p;
		p += dn->type;
		for (size_t i = 0; subto[i]; i++) *q++ = subto[i];
		if (dn->next) { dn = dn->next; goto loop; }
		for (; *p; p++)*q++ = *p;
		*q = 0;
	}
	else ret = StrHeap(dest);

endo:
	DchainDrop(dc);
	mfree(dc);
	aflaga = af;
	return ret;
}
