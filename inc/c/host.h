// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host
// LastCheck: RFZ23
// AllAuthor: @dosconio
// ModuTitle: Host Environment
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

// The file for free environment is the `alice.h`

#ifndef _INC_ENV
#define _INC_ENV

#include "./alice.h"

enum Architecture_t {
	Architecture_x86,
	Architecture_RISCV64,
	
	Architecture_x64,
	Architecture_ARM,
	Architecture_ARM64,
	Architecture_MIPS,
	Architecture_MIPS64,
	Architecture_RISCV32,
	Architecture_POWERPC64,
	Architecture_POWERPC32,
	Architecture_SPARC64,
	Architecture_SPARC32,
	Architecture_Unknown
};

#endif
