// ASCII C99 TAB4 CRLF
// Docutitle: (Module) 
// Codifiers: @dosconio: 20240906 ~ <Last-check> 
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
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

#ifndef _INC_STRPAGE
#define _INC_STRPAGE

#include "stdinc.h"

// assume shift same bits
#define _STRPAGE_SHIFT 12
#define _STRPAGE_SIZE_COUNT (_IMM(1) << _STRPAGE_SHIFT)
#define _STRPAGE_SIZE_LAYER (_STRPAGE_SIZE_COUNT/_BYTE_BITS_*__BITS__)
#define _STRPAGE_SIZE_BLOCK (_STRPAGE_SIZE_COUNT/_BYTE_BITS_*__BITS__)

typedef struct Strpage {
	stduint layer_level;// attached with block, 0 for leaf
	// This may cause much Memory Fragmentation
	union {
		stduint leaves[_STRPAGE_SIZE_COUNT];
		struct Strpage* branches[_STRPAGE_SIZE_COUNT];
	};
} Strpage;

Strpage* StrpageNew(void);

void StrpageFree(Strpage* sp);

stduint StrpageGet(const Strpage* sp, stduint index);

Strpage* StrpageSet(Strpage* sp, stduint index, stduint val);

#endif
