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

// > use `strcat()` is a waste.
// throw: for ReplaceChars
char* StrHeapInsertThrow(const char* d, const char* s, size_t posi, size_t thrown)
{
	char* ret;
	char* ptr;
	size_t dlen, slen;
	size_t i;// for loops below

	dlen = 0; slen = 0;// suitable for VC++2010
	while (s[slen])slen++;// better than strlen stdfunc.
	while (d[dlen])dlen++;
	if (!slen || !dlen || posi + thrown > dlen)
	{
		ret = StrHeap(d);
	}
	// 123456789 throw 56 for ab
	// 1234ab789 posi=4 throw=2 (at most 5) dlen=9 slen=2 
	ret = (char*)malc(dlen + slen - thrown + 1);
	ptr = ret;
	for (i = 0; i < posi; i++)
		*ptr++ = d[i];
	for (i = 0; i < slen; i++)
		*ptr++ = s[i];
	for (i = posi + thrown; i < dlen; i++)
		*ptr++ = d[i];
	*ptr = 0;// 0-TChars
	return ret;
}
