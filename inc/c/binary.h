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

#ifndef _INC__BITS__
#define _INC__BITS__

#include "stdinc.h"
#include "inttypes.h"

#define BitTog(x,bits) ((x)^=(bits))
#define BitSet(dest,bitposi) (dest) |=  (_IMM1 << bitposi)
#define BitClr(dest,bitposi) (dest) &= ~(_IMM1 << bitposi)
#define BitSev(dest,bitposi,val) do{val?BitSet(dest,bitposi):BitClr(dest,bitposi);}while(0)
#define BitGet(dest,bitposi) ((dest) & (_IMM1 << bitposi))

// BitmapSet

// BitmapClear

// BitmapGet



// [Linux does not use this!] Reflect bits of a value like a mirror.
size_t _CALL_FAST BitReflect(size_t times, size_t val);

// 
uint64_t _CALL_CPL BitReflect64(size_t times, uint64_t val);

#endif
