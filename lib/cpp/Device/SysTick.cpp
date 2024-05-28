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

#include "../../../inc/cpp/Device/SysTick"
#include "../../../inc/cpp/Device/NVIC"

#define SysTick_CTRL_CLKSOURCE (1UL << 2)
#define SysTick_CTRL_TICKINT   (1UL << 1) 
#define SysTick_CTRL_ENABLE    (1UL << 0)


namespace uni {
	bool SysTick::enClock(uint32 SysCoreClock, uint32 Hz) {
		NVIC_t nvic;
		SysTick_Map* st = (SysTick_Map*)ADDR_SysTick_Map;
		if (SysCoreClock / Hz > 0xFFFFFF) return false;
		st->LOAD = SysCoreClock / Hz - 1;
		nvic.setPriority(IRQ_SysTick, (1 << _NVIC_PRIO_BITS) - 1);
		st->VAL = 0;// SysTick Counter Value
		st->CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE;// Enable SysTick IRQ and SysTick Timer
		return true;
	}

}

extern "C" {
	volatile stduint delay_count = 0;
	void SysTick_Handler(void) {
		//{TODO} Callback and more options
		delay_count&& delay_count--;
	}
	void SysDelay(stduint unit) {
		//{ISSUE} append systick-enable check?
		delay_count = unit;
		while (delay_count);
	}
}
