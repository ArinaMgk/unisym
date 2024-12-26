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

// initialize items if set when calling SystemInit.
_WEAK bool exist_cortex_m4 = false;
_WEAK bool exist_sram = false;// AKA def: DATA_IN_ExtSRAM (for CM4 but CA7)
_WEAK bool exist_ddr = false;
_WEAK bool exist_fpu = true;
_WEAK bool exist_mmu = false;
_WEAK bool exist_cache = false;

// Vector Table base offset must be a multiple of 0x200

// ONLY A Cortex A7 Chip in MP13 Series, till 2024 Nov.
// MP131 MP133 MP135

extern "C" {
	void SystemInit(void);
	char _IDN_BOARD[16] = "STM32MP13";
}

_ESYM_C{
// extern uint32_t ZI_DATA;
// extern uint32_t __BSS_END__;
//  Ensure all bss part of code is initialized with zeros	
void __attribute__((noinline)) ZeroBss(void)
{
	_ASM volatile(
	"PUSH {R4-R11}          \n"
	"LDR r2, =ZI_DATA       \n"
	"b LoopFillZerobss      \n"
	/* Zero fill the bss segment. */
	"FillZerobss:           \n"
	"MOVS r3, #0            \n"
	"STR  r3, [r2]          \n"
	"adds r2, r2, #4        \n"
	"LoopFillZerobss:       \n"
	"LDR r3, = __BSS_END__  \n"
	"CMP r2, r3             \n"
	"BCC FillZerobss        \n"
	"DSB                    \n"
	"POP    {R4-R11}        "
	);
}
}

#if 0//_TODO
void SystemInit_CortexM4(void) {
	//{} set exist_XXX according to _IDN_BOARD
	//
	// [FPU]
	if (exist_fpu) SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));// set CP10 and CP11 Full Access
	if (exist_sram)// Vector Table Relocation in Internal SRAM
		SCB->VTOR = MCU_AHB_SRAM | VECT_TAB_OFFSET;
	// EXTI[EXTICore::C2].canMode();
	EXTI_C2->IMR1 = 0;
	EXTI_C2->IMR2 = 0;
	EXTI_C2->IMR3 = 0;
	EXTI_C2->EMR1 = 0;
	EXTI_C2->EMR2 = 0;
	EXTI_C2->EMR3 = 0;
}
#endif
// AKA SystemInit_cpu1
void SystemInit_CortexA7(void) {
	using namespace uni;
	MMU.InvalidateTLB();
	L1C.InvalidateBTAC();// Invalidate entire branch predictor array
	L1C.InvalidateICacheAll();// Invalidate instruction cache and flush branch target cache
	L1C.InvalidateDCacheAll();
	if (exist_fpu) {
		FPU.enAble();
	}
	if (exist_mmu) {
		MMU.CreateTranslationTable();
		MMU.enAble();
	}
	if (exist_cache) {
		L1C.enAble();
	}
	L1C.enBTAC();
}

void SystemInit(void) {
	using namespace uni;
	ZeroBss();
	EXTI[EXTICore::C1].canMode();
	SystemInit_CortexA7();
	IRQ_Initialize();
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
	int _close(int file);
	int _lseek(int file, int ptr, int dir);
	int _read(int file, char* ptr, int len);
	int _write(int file, char* ptr, int len);

}


_WEAK int _getpid(void)
{
	return 1;
}

_WEAK int _kill(int pid, int sig)
{
	(void)pid;
	(void)sig;
	//{} errno = EINVAL;
	return -1;
}

_WEAK void _exit(int status)
{
	_kill(status, -1);
	while (1);
}

_WEAK int _close(int file)
{
	(void)file;
	return -1;
}

_WEAK int _lseek(int file, int ptr, int dir)
{
	(void)file; (void)ptr; (void)dir;
	return -1;
}

_WEAK int _read(int file, char* ptr, int len)
{
	(void)file; (void)ptr; (void)len;
	return -1;
}

_WEAK int _write(int file, char* ptr, int len)
{
	(void)file; (void)ptr; (void)len;
	return -1;
}

// #ifdef DATA_IN_ExtSRAM
_ESYM_C _WEAK void SystemInit_ExtMemCtl(void) { _TODO } // for MP15.CM4

#endif
