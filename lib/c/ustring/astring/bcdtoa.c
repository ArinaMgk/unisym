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

char* _Need_free bcdtoa(unsigned char* str, size_t bylen)
{
	// started from RedToLocaleClassic()
	// 9876_5432H <=== 0x32,0x54,0x76,0x98;
	// 9876.5432H <=== 0x32,0x54`0x76,0x98;
	register char* res = (char*)malc((bylen << 1) | 1);
	char c;
	res += (bylen << 1);
	*res-- = 0;
	for (size_t i = 0; i < bylen; i++)
	{
		c = str[i] & 0x0F;
		*res-- = c >= 10 ? c - 10 + 'A' : c + '0';
		str[i] >>= 4;
		c = str[i] & 0x0F;
		*res-- = c >= 10 ? c - 10 + 'A' : c + '0';
	}
	return res + 1;
}
