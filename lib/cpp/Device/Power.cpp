// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) Power Control (PWR and Watchdog)
// Codifiers: @dosconio: 20241119
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

#include "../../../inc/cpp/Device/_Power.hpp"

namespace uni {
#if 0

#elif defined(_MCU_STM32H7x)
#include "../../../inc/cpp/Device/Power/Power-STM32H7.hpp"
	PWR_t PWR;
	void PWR_t::ConfigVoltageScaling(byte regulator) {
		PWR_D3CR_VOS = regulator;
		stduint tmpreg = PWR_D3CR_VOS;// Delay
	}

#elif defined(_MPU_STM32MP13)
	PWR_t PWR;
#elif defined(_MCU_MSP432P4)
	#include "../../../inc/c/MCU/MSP432/MSP432P4.h"
	Watchdog_t WdogA;

	Reference_T<uint16> Watchdog_t::operator[](WDogReg idx) {
		return WDT_A_BASE + _IMMx2(idx);
	}
	
	void Watchdog_t::HoldTimer() {
		ROM_WDTTABLE[0]();
	}

	
#endif
}

