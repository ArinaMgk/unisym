// ASCII C++-11 TAB4 CRLF
// LastCheck: 20241115
// AllAuthor: @dosconio
// ModuTitle: (MCU) STM32MP13 Series
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

/* also act as implementation of `system_stm32f4xx.c`
- SystemInit() <-> SystemInit()
- SystemCoreClock <-> uni::SystemCoreClock yo RCC.cpp
- SystemCoreClockUpdate() <-> RCC.Sysclock.getCoreFrequency()
*/

#include "../../../inc/c/stdinc.h"
#ifdef _MPU_STM32MP13

#include "../../../inc/c/prochip/CortexA7.h"
#include "../../../inc/cpp/MCU/ST/STM32MP13"

extern "C" void SystemInit(void);

extern "C" {
	char _IDN_BOARD[16] = "STM32MP13";
}

void _TODO_SystemInit(void) {

}


namespace uni {


}

#endif
