// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant 
// LastCheck: RFZ22
// AllAuthor: @dosconio
// ModuTitle: Operations for ASCIZ Character-based String
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

void _size_decimal_get();

ptrdiff_t atoins(const char* str)
{
	int sign = 0; int Signed = 0;
	///if (!str || !*str)return 0;
	if (*str == '-') sign++, str++, Signed++;
	else if (*str == '+') str++, Signed++;
	if (!_size_decimal) _size_decimal_get();
	const char* ptr = str;
	ptrdiff_t inst = 0;
	while (*ptr && *ptr <= '9' && *ptr >= '0' && (size_t)(ptr - str + !Signed) <= _size_decimal)
	{
		inst *= 10;
		inst += (ptrdiff_t)*ptr - (ptrdiff_t)'0';
		ptr++;
	}
	///if (*ptr <= '9' && *ptr >= '0') ...
	return sign ? -inst : inst;
}
