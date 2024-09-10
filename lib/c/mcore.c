// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ06
// AllAuthor: @dosconio
// ModuTitle: Micro Core for UNISYM
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

#include "../../inc/c/stdinc.h"

struct _aflag_t aflaga = { .autosort = 1 };

char _tab_HEXA[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
char _tab_hexa[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

// ---- { memory } ----
#ifdef _DEBUG
size_t malc_count = 0;
size_t malc_limit = _MALLIMIT_DEFAULT;// you can use such `malc_limit = 100;` to change this.
#endif
#ifndef _MCCA
void memf(void* m) { memfree(m); }
#endif
// ---- { calling } ----

size_t call_state;
