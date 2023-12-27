// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant CPU(80586+)
// LastCheck: RFZ04
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

#ifndef _INC_BINARY
#define _INC_BINARY

#include "alice.h"
#include "inttypes.h"

#define BitTog(x,bits) ((x)^=(bits))

// Reflect bits of a value like a mirror.
size_t __fastcall BitReflect(size_t times, size_t val);

// 
uint64_t __cdecl BitReflect64(size_t times, uint64_t val);

#endif
