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

#include "../../../../inc/c/aldbg.h"
#include "../../../../inc/c/ustring.h"

unsigned char* _Need_free atobcd(char* str)
{
	// 98765432H ===> 0x32,0x54,0x76,0x98;
	// keep the str even, so you may allocate an odd size.
	// Without input check
	size_t slen = StrLength(str);
	if (slen & 1) return 0;
	slen >>= 1;
	unsigned char CrtChr = 0, * res = (unsigned char*)malc(slen + 1);
	res[slen] = 0;
	size_t i = slen;
	do
	{
		CrtChr = 0;
		if (str[1] >= 'A') CrtChr = str[1] - 'A';
		else CrtChr = str[1] - '0';
		if (str[0] >= 'A') CrtChr |= (str[0] - 'A') << 4;
		else CrtChr |= (str[0] - '0') << 4;
		str += 2;
		i--;
		res[i] = CrtChr;
	} while (i > 0);
	return res;
}
