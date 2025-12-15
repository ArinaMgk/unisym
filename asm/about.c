// ASCII DJGPP (Debug MSVC2010) TAB4 CRLF
// Attribute: ArnCovenant Host bin^16+ 386+
// LastCheck: RFX19
// AllAuthor: @dosconio
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

#define _AUTO_INCLUDE
#define __BUILDENV__
#include "inc/aasm.h"
#include <../../demo/template/version/version.h>

//
#define AASM_VER "1.00"

const char aasm_version[] = AASM_VER;// aka NASM 2.07
const char aasm_date[] = __DATE__;
const char aasm_compile_options[] = ""
#ifdef _DEBUG
" with -D_DEBUG"
#endif
;

const char aasm_comment[] =
"The Arinae Assembler " AASM_VER;

const char aasm_signature[] =
"NASMANL " AASM_VER;
