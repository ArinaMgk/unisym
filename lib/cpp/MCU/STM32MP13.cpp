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

#include "../../../inc/c/stdinc.h"
#ifdef _MPU_STM32MP13

#include "../../../inc/c/prochip/CortexA7.h"
#include "../../../inc/cpp/MCU/ST/STM32MP13"

// ONLY A Cortex A7 Chip in MP13 Series, till 2024 Nov.
// MP131 MP133 MP135

extern "C" void SystemInit(void);
extern "C" void MP13_SystemInit(void);

extern "C" {
	char _IDN_BOARD[16] = "STM32MP13";
}

void SystemInit(void) {
	MP13_SystemInit();
}


namespace uni {


}

_ESYM_C _WEAK void SystemInit_Interrupts_SoftIRQn_Handler(uint32_t Software_Interrupt_Id, uint8_t cpu_id_request)
{
	((void)Software_Interrupt_Id);
	((void)cpu_id_request);
}

// serve for libc_nano.a in bare-metal mode
extern "C" {
	int _getpid(void);
	int _kill(int pid, int sig);
	void _exit(int status);
}


int _getpid(void)
{
	return 1;
}

int _kill(int pid, int sig)
{
	(void)pid;
	(void)sig;
	//{} errno = EINVAL;
	return -1;
}

void _exit(int status)
{
	_kill(status, -1);
	while (1);
}

#endif
