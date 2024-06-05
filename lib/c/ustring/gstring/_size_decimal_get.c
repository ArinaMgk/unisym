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

#define DECLEN_1B  1 // ~0 = 1
#define DECLEN_4B  2 // ~0 = 16
#define DECLEN_8B  3 // ~0 = 255
#define DECLEN_16B 5 // ~0 = 65535
#define DECLEN_32B 10//      4,294,967,295
#define DECLEN_64B 20//      18,446,744,073,709,551,615
unsigned _size_decimal_table[] = { DECLEN_1B,DECLEN_4B,DECLEN_8B,DECLEN_16B, DECLEN_32B, DECLEN_64B };

//{OUTDATED TODEL}size_t _size_decimal = 0;

size_t _size_decimal_get()
{
	if (sizeof(ptrdiff_t) <= numsof(_size_decimal_table))
		return lookupDecimalDigits(sizeof(ptrdiff_t));
	_REGISTER unsigned int i = 0;
	_REGISTER size_t r = (size_t)~0;
	while (r)
		i++, r /= 10;
	//x64 for 20, and x86 for 10
	return i;
}

size_t lookupDecimalDigits(size_t expo) {
	return _size_decimal_table[expo]; //{TODO} assert
}
