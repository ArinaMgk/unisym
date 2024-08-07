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

//{TOD} check
// From left, one by one, return all in heap! must free() by yourself. Inputs won't be free!
// RFV07 rename from "StrReplaceHeap"
char* StrReplace(const char* dest, const char* subfirstrom, const char* subto, size_t* times)
{
	struct _aflag_t af = aflaga;

	if (times) *times = 0;
	if (!dest || !subfirstrom || !subto || !malc_limit)
		return zalc(1);
	if (!*dest || !*subfirstrom) return StrHeap(dest);
	Dnode* dn = 0;
	size_t sz_subto = 0, nums = 0, sz_subfirstrom = 0, sz_len = 0;
	ptrdiff_t chars_add = 0;
	for (; subto[sz_subto]; sz_subto++);
	for (; subfirstrom[sz_subfirstrom]; sz_subfirstrom++);
	for (; dest[sz_len]; sz_len++);
	chars_add = (ptrdiff_t)sz_subto - (ptrdiff_t)sz_subfirstrom;
	const char* p = dest;
	while (1)
	{
		p = StrIndexString(p, subfirstrom);
		if (!p) break;
		dn = DnodeInsert(dn, (void*)p, sz_subfirstrom, 0, 1/*ON_RIGHT*/);
		nums++;
		p += sz_subfirstrom;
	}
	if (nums)
	{
		if (times) *times = nums;
		char* ret = malc((ptrdiff_t)sz_len + (ptrdiff_t)chars_add + (ptrdiff_t)1);
		ret[(ptrdiff_t)sz_len + (ptrdiff_t)chars_add] = 0;// for dbg
		p = dest; char* q = ret;
		dn = DnodeRewind(dn);
	loop:
		for (; p < dn->addr; p++) *q++ = *p;
		p += dn->type;
		for (size_t i = 0; subto[i]; i++) *q++ = subto[i];
		if (dn->next) { dn = dn->next; goto loop; }
		for (; *p; p++)*q++ = *p;
		*q = 0;
		DnodesRelease(dn, 0);// non-free
		return ret;
	}
	else
		return StrHeap(dest);

endo:
	aflaga = af;
	return 0;
}
