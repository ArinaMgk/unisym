// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ05
// AllAuthor: @dosconio
// ModuTitle: Binary Digit
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

#include "../../inc/c/binary.h"

uint64_t _CALL_CPL BitReflect64(size_t times, uint64_t val)
{
	uint64_t reflection = 0x0;
	if (!times) return val;
	for (int bit = 0; bit < times; bit++)
	{
		if (val & 0x01)
			reflection |= (1ULL << (times - 1 - bit));
		val >>= 1;
	}
	return reflection;
}
