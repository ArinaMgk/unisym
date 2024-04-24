// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ23
// AllAuthor: @ArinaMgk
// ModuTitle: String Pool
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

#ifndef _LIB_STRPOOL
#define _LIB_STRPOOL

#include "stdinc.h"

/* ROUTINE EXAMPLE
	StrPoolInit();
	atexit(StrPoolRelease);
	printf("%s", str = StrPoolHeap("Hello,world!"));
*/
#define _ModString_Strpool_UNIT_SIZE 4096// A page align by default

//
void StrPoolInit();
//
char* StrPoolHeap(const char*, size_t);
//
char* StrPoolAlloc(size_t);
//
char* StrPoolAllocZero(size_t);
//
void StrPoolRelease();

#endif
