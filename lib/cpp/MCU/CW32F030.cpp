// ASCII C++-11 TAB4 CRLF
// LastCheck: 20240826
// AllAuthor: @dosconio
// ModuTitle: (MCU) ARM Cortex M0+ CW32F030
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

/* also act as implementation of `system_cw32f030.c`
- SystemCoreClockUpdate
- SystemInit
*/
#include "../../../inc/c/stdinc.h"
#ifdef _MCU_CW32F030

#define _OPT_PCU_CortexM0P
#include "../../../inc/c/prochip/CortexM0.h"// M0+

extern "C" void SystemInit(void);

extern "C" char _IDN_BOARD[16];
_WEAK char _IDN_BOARD[16] = "CW32F030";

extern "C" stduint SystemCoreClock;

void SystemInit(void) {
	_TEMP Letvar(hsi, volatile uint32 *, 0x40010018UL);
	_TEMP Letvar(lsi, volatile uint32 *, 0x40010020UL);
	//: CW_SYSCTRL->HSI_f.TRIM = *((volatile uint16_t*)RCC_HSI_TRIMCODEADDR);
	*hsi = (*hsi & ~_IMM(0x7FF)) | 0x00012600U;
	//: CW_SYSCTRL->LSI_f.TRIM = *((volatile uint16_t*)RCC_LSI_TRIMCODEADDR);
	*lsi = (*lsi & ~_IMM(0x3FF)) | 0x00012602U;
}

namespace uni {

}

#endif
