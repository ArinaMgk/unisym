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

#ifndef _INC_DEVICE_PWR
#define _INC_DEVICE_PWR

#include "../unisym"
#include "../reference"

#if defined(_MCU_STM32H7x)
#define _ADDR_PWR (D3_AHB1PERIPH_BASE + 0x4800)
#elif defined(_MPU_STM32MP13)
#define _ADDR_PWR 0x50001000
#endif

namespace uni {
#if 0
	
#elif defined(_MCU_STM32H7x)

#include "../MCU/_ADDRESS/ADDR-STM32.h"
#ifdef _MCU_PWR_TEMP
#include "./Power/Power-STM32H7.hpp"
#endif

enum class PWRReg {
	CR1, CSR1, CR2, CR3,
	CPUCR,
	D3CR = 0x18 / 4,
	WKUPCR = 0x20 / 4, WKUPFR, WKUPEPR,
};

class PWR_t {
public:
	Reference operator[](PWRReg idx) {
		return _ADDR_PWR + _IMMx4(idx);
	}

	// TEMP AREA

	// Configure the main internal regulator output voltage, AKA __HAL_PWR_VOLTAGESCALING_CONFIG
	// @param regulator: `[0 SCALE3, 1 SCALE2, 3 SCALE1]` the regulator output voltage to achieve a tradeoff between performance and power consumption when the device does not operate at the maximum frequency (refer to the datasheets for more details).
	void ConfigVoltageScaling(byte regulator);


};
extern PWR_t PWR;

#elif defined(_MPU_STM32MP13)

	enum class PWRReg {
		CR1 = 0, CSR1, CR2, CR3,
		MPUCR,
		WKUPCR = 0x20 / 4, WKUPFR, MPUWKUPENR,
		VERR = 0x3F4/4, ID, SID
	};

	class PWR_t {
	public:
		Reference operator[](PWRReg idx) {
			return _ADDR_PWR + _IMMx4(idx);
		}

		// TEMP AREA

		void setDBP(bool ena) {
			self[PWRReg::CR1].setof(8);// DBP
			while (ena ^ self[PWRReg::CR1].bitof(8));
		}
	};
	extern PWR_t PWR;

#elif defined(_MCU_MSP432P4)

	enum class WDogReg {
		CTL = 6,
	};
	class Watchdog_t {
	public:
		Reference_T<uint16> operator[](WDogReg idx);
		// AKA ROM_WDT_A_holdTimer
		void HoldTimer();
	};

	extern Watchdog_t WdogA;// Watchdog A
#endif
}

#endif
