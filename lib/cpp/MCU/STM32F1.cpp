// ASCII C++-11 TAB4 CRLF
// LastCheck: 20240826
// AllAuthor: @dosconio
// ModuTitle: (MCU) STM32F4 Series
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

/* also act as implementation of `system_stm32f1xx.c`
- SystemInit() <-> SystemInit()
- SystemCoreClock <-> uni::SystemCoreClock yo RCC.cpp
- SystemCoreClockUpdate() <-> uni::SystemCoreClock = RCC.Sysclock.getFrequency() >> AHBPrescTable[(RCC[CFGR] & _RCC_CFGR_MASK_HPRE) >> _RCC_CFGR_POSI_HPRE];
*/
#include "../../../inc/c/stdinc.h"
#ifdef _MCU_STM32F1x

#include "../../../inc/c/prochip/CortexM3.h"
#include "../../../inc/cpp/MCU/ST/STM32F1"

extern "C" void SystemInit(void);

extern "C" {
	char _IDN_BOARD[16] = "xx";// fill this with "03VET6" for STM32F103VET6 (20240826 came up)
}

void SystemInit(void) {
	// No FPU for STM32F1
	// External S(D)RAM
	//{TODO}
	// User's Vector Table
	//{TODO}
}

namespace uni {




	// void SystemInit_ExtMemCtl(void);//{TODO}
}

#endif
