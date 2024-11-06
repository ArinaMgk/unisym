// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) Bitmap, not a picture format
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
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

#ifndef _INC_BITMAP
#define _INC_BITMAP
#include "stdinc.h"
#ifdef _INC_CPP

namespace uni {
	class Bitmap {
	protected:
		pureptr_t offs;// align by byte
	public:
		Bitmap(pureptr_t offs) : offs(offs) {}
		/*//{TODO}
		- setof() making use of ASM inst.
		*/
	};
}


#endif
#endif
