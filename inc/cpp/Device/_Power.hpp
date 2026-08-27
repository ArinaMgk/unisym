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

// low power mode entered by setMode
enum class PWRMode : byte { SLEEP, STOP, STANDBY };
// power domain (D1/D2/D3), for domain-scoped low-power / D3 config
enum class PWRDomain : byte { D1, D2, D3 };
// low power mode entry instruction
enum class PWREntry : byte { WFI, WFE };
// regulator state in STOP mode (maps to CR1.LPDS)
enum class PWRRegulator : byte { MAIN, LOWPOWER };
// main regulator voltage scaling (D3CR.VOS value)
enum class PWRVoltageScale : byte { SCALE3 = 1, SCALE2 = 2, SCALE1 = 3 };
// stop mode voltage scaling (CR1.SVOS value)
enum class PWRStopVoltageScale : byte { SCALE3 = 1, SCALE4 = 2, SCALE5 = 3 };
// wake-up pin identity (WKUPEPR/WKUPFR bit position)
enum class PWRWakeUpPin : byte { WKUP1 = 0, WKUP2 = 1, WKUP3 = 2, WKUP4 = 3, WKUP5 = 4, WKUP6 = 5 };
// monitored level result (temperature / VBAT)
enum class PWRMonitorLevel : byte { BELOW_LOW, BETWEEN, ABOVE_HIGH };

// wake-up pin configuration (mirrors HAL PWREx_WakeupPinTypeDef)
struct PWRWakeUpPinConfig {
	PWRWakeUpPin pin;
	byte polarity;// 0 high/rising, 1 low/falling
	byte pull;// 0 none, 1 pull-up, 2 pull-down
};

class PWR_t {
public:
	Reference operator[](PWRReg idx) {
		return _ADDR_PWR + _IMMx4(idx);
	}

	// Configure the main internal regulator output voltage, AKA __HAL_PWR_VOLTAGESCALING_CONFIG
	// @param regulator: `[0 SCALE3, 1 SCALE2, 3 SCALE1]` the regulator output voltage to achieve a tradeoff between performance and power consumption when the device does not operate at the maximum frequency (refer to the datasheets for more details).
	void ConfigVoltageScaling(byte regulator);

	// ---- low power mode entry ----
	// AKA HAL_PWR_EnterSLEEPMode / HAL_PWR_EnterSTOPMode / HAL_PWR_EnterSTANDBYMode
	//      HAL_PWREx_EnterSTOPMode / HAL_PWREx_EnterSTANDBYMode
	void setMode(PWRMode mode, PWREntry entry = PWREntry::WFI, PWRRegulator regulator = PWRRegulator::MAIN, PWRDomain domain = PWRDomain::D1);

	// ---- voltage / supply ----
	// AKA HAL_PWREx_ControlVoltageScaling
	bool ControlVoltageScaling(PWRVoltageScale scaling);
	// AKA HAL_PWREx_GetVoltageRange
	stduint getVoltageRange();
	// AKA HAL_PWREx_ControlStopModeVoltageScaling
	void ControlStopModeVoltageScaling(PWRStopVoltageScale scaling);
	// AKA HAL_PWREx_GetStopModeVoltageRange
	stduint getStopModeVoltageRange();
	// AKA HAL_PWREx_ConfigSupply
	bool ConfigSupply(stduint supply);
	// AKA HAL_PWREx_GetSupplyConfig
	stduint getSupplyConfig();

	// ---- backup domain ----
	// AKA HAL_PWR_EnableBkUpAccess / DisableBkUpAccess
	void enBkUpAccess(bool ena = true);
	// AKA HAL_PWREx_EnableBkUpReg / DisableBkUpReg
	bool enBkUpReg(bool ena = true);

	// ---- wake-up pin ----
	// AKA HAL_PWREx_EnableWakeUpPin / DisableWakeUpPin (and legacy HAL_PWR_EnableWakeUpPin)
	void enWakeUpPin(const PWRWakeUpPinConfig& cfg);
	// AKA HAL_PWREx_GetWakeupFlag
	stduint getWakeupFlag(stduint flag);
	// AKA HAL_PWREx_ClearWakeupFlag
	bool clearWakeupFlag(stduint flag);

	// ---- voltage detectors ----
	// AKA HAL_PWR_ConfigPVD (threshold level only; EXTI config belongs to EXTI layer)
	void ConfigProgrammableVoltageDetector(stduint level);
	// AKA HAL_PWR_EnablePVD / DisablePVD
	void enProgrammableVoltageDetector(bool ena = true);
	// AKA HAL_PWREx_ConfigAVD (threshold level only)
	void ConfigAnalogVoltageDetector(stduint level);
	// AKA HAL_PWREx_EnableAVD / DisableAVD
	void enAnalogVoltageDetector(bool ena = true);

	// ---- Cortex SCR bits ----
	// AKA HAL_PWR_EnableSleepOnExit / DisableSleepOnExit
	void enSleepOnExit(bool ena = true);
	// AKA HAL_PWR_EnableSEVOnPend / DisableSEVOnPend
	void enSEVOnPend(bool ena = true);

	// ---- power features ----
	// AKA HAL_PWREx_ConfigD3Domain
	void ConfigD3Domain(stduint state);
	// AKA HAL_PWREx_EnableFlashPowerDown / DisableFlashPowerDown
	void enFlashPowerDown(bool ena = true);
	// AKA HAL_PWREx_EnableUSBReg / DisableUSBReg
	bool enUSBReg(bool ena = true);
	// AKA HAL_PWREx_EnableUSBVoltageDetector / DisableUSBVoltageDetector
	void enUSBVoltageDetector(bool ena = true);
	// AKA HAL_PWREx_EnableBatteryCharging / DisableBatteryCharging
	void enBatteryCharging(bool ena, stduint resistor = 0);
	// AKA HAL_PWREx_EnableMonitoring / DisableMonitoring
	void enMonitoring(bool ena = true);

	// ---- monitoring result ----
	// AKA HAL_PWREx_GetTemperatureLevel
	PWRMonitorLevel getTemperatureLevel();
	// AKA HAL_PWREx_GetVBATLevel
	PWRMonitorLevel getVBATLevel();

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
