// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant 
// LastCheck: RFZ22
// AllAuthor: @dosconio
// ModuTitle: Operations for ChrAr
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

// Clear prefix zeros of hexa. E.g. "00012500" >>> "12500", "-00" >>> "-0"
// No need to make a specific buffer-version
size_t ChrCpz(char* str)
{
	// assume str:not-null
	if (*str == '+' || *str == '-') str++;
	size_t siz = StrLength(str);
	ptrdiff_t num = 0;
	char c;
	while ((c = str[num]) && c == '0') num++;
	if (!num) return 0;
	if (c == 0 && str[num - 1] == '0')
	{
		str[1] = 0;
		return num - 1;
	}
	else MemRelative(str + num, siz - num + 1, -num);// 000905_ : str=0 num=3 siz=6
	return num;
}
