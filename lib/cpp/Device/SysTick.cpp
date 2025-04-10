// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) SysTick
// Codifiers: @dosconio: 20240511
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

#include "../../../inc/cpp/Device/RCC/RCC"
#include "../../../inc/cpp/Device/SysTick"
#include "../../../inc/cpp/Device/NVIC"


#define SysTick_CTRL_CLKSOURCE (1UL << 2)
#define SysTick_CTRL_TICKINT   (1UL << 1) 
#define SysTick_CTRL_ENABLE    (1UL << 0)

stduint SysTickHz = 1000;

#ifdef _MCU_STM32F1x
namespace uni {
	bool SysTick::enClock(uint32 Hz) {
		NVIC_t nvic;
		SysTick_Map* st = (SysTick_Map*)ADDR_SysTick_Map;
		if (SystemCoreClock / Hz > 0xFFFFFF) return false;
		st->LOAD = SystemCoreClock / Hz - 1;
		nvic.setPriority(IRQ_SysTick, (1 << _NVIC_PRIO_BITS) - 1);
		st->VAL = 0;// SysTick Counter Value
		st->CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE;// Enable SysTick IRQ and SysTick Timer
		return true;
	}

}

#elif defined(_MCU_STM32F4x)

namespace uni {
	bool SysTick::enClock(uint32 Hz) {
		//aka HAL_SYSTICK_Config core_cm4::SysTick_Config
		if (SystemCoreClock / Hz - 1 > 0xFFFFFF/*wo CortexM4*/) return false;
		ref().LOAD = SystemCoreClock / Hz - 1;
		NVIC.setPriority(IRQ_SysTick, (1 << _NVIC_PRIO_BITS) - 1);
		ref().VAL = 0;
		ref().CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE;
		return true;
	}
}
#elif defined(_MCU_STM32H7x)

namespace uni {
	bool SysTick::enClock(uint32 _Hz) {
		if (!_Hz || (SystemCoreClock / _Hz - 1 > SysTick_LOAD_RELOAD_Msk)) return false;
		SysTickHz = _Hz;
		//aka HAL_SYSTICK_Config SysTick_Config
		ref().LOAD = SystemCoreClock / _Hz - 1; /* set reload register */
		NVIC.setPriority(IRQ_SysTick, (1 << _NVIC_PRIO_BITS) - 1);
		ref().VAL = 0UL; /* Load the SysTick Counter Value */
		ref().CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk | 4; // SYSTICK_CLKSOURCE_HCLK;
		return true;
	}
}
#elif defined(_MPU_STM32MP13)

#include "../../../inc/c/proctrl/ARM/cortex_a7.h"


extern stduint HSE_VALUE, HSI_VALUE;
namespace uni {
	bool SysTick::enClock(uint32 _Hz) {
		_TEMP stduint TICK_INT_PRIORITY = 0x0FU;
		if (!_Hz) return false;
		SysTickHz = _Hz;
		if (0 _TODO) { // defined(USE_ST_CASIS)
			//{TODO} HAL_SYSTICK_Config(SystemCoreClock / Hz);
		}
		else if (0 _TODO) { // def (USE_PL1_SecurePhysicalTimer_IRQ)
			//{TODO}
		}
		else {
			// Cortex Related
			// PL1_SetCounterFrequency(HSI_VALUE);// Set Counter Frequency {__set_CNTFRQ(HSI_VALUE);+__ISB}
			{
				__set_CP(15, 0, HSI_VALUE, 14, 0, 0);
				__ISB();
			}
			// PL1_SetLoadValue(0x1U);// Initialize Counter {__set_CNTP_TVAL(0x1U);+__ISB}
			{
				__set_CP(15, 0, 0x1U, 14, 2, 0);
				__ISB();
			}
		}
		return true;
	}

	uint64 SysTick::getTickPhysical() {
		uint64 PL1_GetCurrentPhysicalValue;// AKA IT
		__get_CP64(15, 0, PL1_GetCurrentPhysicalValue, 14);
		return PL1_GetCurrentPhysicalValue;
	}
	
	uint64 SysTick::getTick() {
		using namespace RCCReg;
		bool hse = 0x1/*HSE*/ == RCC[STGENCKSELR].masof(0, 2);// STGENSRC
		return getTickPhysical() / ((hse ? HSE_VALUE : HSI_VALUE) / SysTickHz);
	}
}

#endif

extern "C" {
	extern volatile stduint delay_count;
	void SysTick_Handler(void);
}

volatile stduint delay_count = 0;
void SysTick_Handler(void) {
	//{TODO} Callback and more options
	asserv(delay_count)--;
}
void SysDelay(stduint unit) {
#if defined(_MPU_STM32MP13)
	using namespace uni;
	uint64 endo = SysTick::getTick() + unit;
	while (true) if (SysTick::getTick() >= endo) break;
#elif defined(_MCU_STM32H7x)
	uint32 tcnt = 0;
	uint32 tick_load = uni::SysTick::ref().LOAD;
	uint32 tick_prev = uni::SysTick::ref().VAL;
	uint64 dest = (uint64)unit * (SystemCoreClock / SysTickHz);
	while (true) {
		uint32 tick_last = uni::SysTick::ref().VAL;
		if (tick_last != tick_prev) {
			if (tick_last < tick_prev) tcnt += tick_prev - tick_last;
			else tcnt += tick_load - tick_last + tick_prev;
			tick_prev = tick_last;
			if (tcnt >= dest) break;
		}
	}

#else
	//{ISSUE} append systick-enable check?
	delay_count = unit;
	while (delay_count);
#endif
}
void SysDelay_ms(stduint ms) {
	SysDelay(ms * SysTickHz / 1000);
}

void SysDelay_us(stduint us) {
	if (us * SysTickHz < 1000000) {
		for (volatile stduint i = 0; i < SystemCoreClock / 1000000 * us; i++);
	}
	else SysDelay(us * SysTickHz / 1000000);
}

#if defined(_MPU_STM32MP13)
void SecurePhysicalTimer_IRQHandler(void) {
	uni::GIC[IRQ_SecurePhyTimer].setPending(false);
	/*
		_ASM volatile(	"MRC p15, 0, r3, c14, c2, 0" "\n"
						"ADD r3, r3, #47872"         "\n"
						"ADD r3, r3, #128"           "\n"
						"MCR p15, 0, r3, c14, c2, 0":::"r3");
			Measured error with (-O0) is 6 CLK/tick = 6/48000 = 0.01 % = 0.1ms/s (34 CLK/tick ... 0.7ms/s for C)
			Measured error with (-O2) is 6 CLK/tick = 6/48000 = 0.01 % = 0.1ms/s
	*/
	PL1_SetLoadValue((HSI_VALUE / 1000U) +
		PL1_GetCurrentValue()// compensates. decrements below 0 after IRQ trigger, negative time since triggered
	);
	SysTick_Handler();// HAL_IncTick
}// TICK
void NonSecurePhysicalTimer_IRQHandler(void) {}
#endif
