// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module&Device) Pseudo-random
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

#ifndef _INC_Pseudo_Random
#define _INC_Pseudo_Random

#include "stdinc.h"

_ESYM_C int rand(void);
_ESYM_C void srand(unsigned int seed);

#if defined(_MPU_STM32MP13) && defined(_INC_CPP)
namespace uni {

	class RNG {
		//{TODO}
	};
	
}
#endif

#endif
