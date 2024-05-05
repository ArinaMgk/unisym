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

#include "../../../../inc/c/ustring.h"

// In the direction of the left
stdint MemCompareRight(const unsigned char* a, const unsigned char* b, size_t n)
{
	for (ptrdiff_t i = n - 1; i >= 0; i--)
	{
		stdint state = (unsigned int)a[i] - (unsigned int)b[i];
		if (state) return state;
	}
	return 0;
}
