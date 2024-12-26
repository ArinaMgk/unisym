// ASCII C++-11 TAB4 CRLF
// LastCheck: 20241029
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

#include "../../../inc/c/stdinc.h"
#ifdef _MCU_STM32F4x

#include "../../../inc/c/prochip/CortexM4.h"
#include "../../../inc/cpp/MCU/ST/STM32F4"

extern "C" void SystemInit(void);

extern "C" {
	char _IDN_BOARD[16] = "STM32F4";
}

void SystemInit(void) {
	// FPU
	((SysCtrlBlock_Map*)_SCB_BASE)->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));// set CP10 and CP11 Full Access
	// External S(D)RAM
	//{TODO}
	// User's Vector Table
	//{TODO}
}


namespace uni {

	// each takes 0x400 area memory
	USART_t XART1(0x40011000, 1);

	USART_t XART2(0x40004400, 2);
	USART_t XART3(0x40004800, 3);
	UART_t  XART4(0x40004C00, 4);
	UART_t  XART5(0x40005000, 5);
	
	USART_t XART6(0x40011400, 6);
	UART_t  XART7(0x40007800, 7);
	UART_t  XART8(0x40007C00, 8);

	
}

#endif
