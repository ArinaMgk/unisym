// ASCII C++-11 TAB4 CRLF
// LastCheck: 20240430
// AllAuthor: @dosconio
// ModuTitle: Integer based on ChrA/Br
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
- SystemCoreClockUpdate() <-> uni::SystemCoreClock = RCC.Sysclock.getFrequency() >> AHBPrescTable[(RCC[CFGR] & _RCC_CFGR_MASK_HPRE) >> _RCC_CFGR_POSI_HPRE];
*/
#include "../../../inc/c/prochip/CortexM4.h"
#include "../../../inc/cpp/MCU/ST/STM32F4"

extern "C" void SystemInit(void);

void SystemInit(void) {
	// FPU
	((SysCtrlBlock_Map*)_SCB_BASE)->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));// set CP10 and CP11 Full Access
	// External S(D)RAM
	//{TODO}
	// User's Vector Table
	//{TODO}
}


namespace uni {

	// takes 0x400 area memory
	USART_t XART1(0x40011000, 1);
	USART_t XART2(0x40004400, 2);
	USART_t XART3(0x40004800, 3);
	UART_t  XART4(0x40004C00, 4);
	UART_t  XART5(0x40005000, 5);
	USART_t XART6(0x40011400, 6);
	UART_t  XART7(0x40007800, 7);
	UART_t  XART8(0x40007C00, 8);

	
}
