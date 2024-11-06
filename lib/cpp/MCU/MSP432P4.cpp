// ASCII C++-11 TAB4 CRLF
// LastCheck: 20241007
// AllAuthor: @dosconio
// ModuTitle: (MCU) Texas Instruments MSP432P4x
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

#include "../../../inc/c/stdinc.h"
#ifdef _MCU_MSP432P4

#define _OPT_PCU_CortexM4F
#include "../../../inc/c/prochip/CortexM4.h"
#include "../../../inc/cpp/MCU/TI/MSP432P4"

extern "C" void SystemInit(void);

extern "C" char _IDN_BOARD[16];
char _IDN_BOARD[16] = "xx";// e.g. "01R" for MSP-EXP432P401R

void SystemInit(void) {
	_TODO
}


namespace uni {


}

#endif
