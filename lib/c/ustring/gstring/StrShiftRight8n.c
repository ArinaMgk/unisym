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

void StrShiftRight8n(void* s, size_t len, size_t n)
{
	///MemRelative(s, len, -(ptrdiff_t)n);
	register size_t i = 0;
	if (len == 0 || n == 0) return;
	if (n >= len) MemSet(s, 0, len);
	else
	{
		while (i < len) { if (i >= n) *(((unsigned char*)s) + i - n) = ((unsigned char*)s)[i]; i++; }
		for (size_t j = 0; j < n; j++) (((unsigned char*)s) - n + len)[j] = 0;
	}
}
