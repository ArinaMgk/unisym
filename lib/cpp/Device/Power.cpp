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
#include "../../../inc/cpp/Device/SysTick"
#include "../../../inc/cpp/Device/EXTI"

namespace uni {
#if 0

#elif defined(_MCU_STM32H7x)
#include "../../../inc/cpp/Device/Power/Power-STM32H7.hpp"

	PWR_t PWR;

	// SCB SCR register (System Control Register) and its low-power bits
	static inline volatile uint32& _SCB_SCR() { return *(volatile uint32*)0xE000ED10; }
	#define _SCR_SLEEPONEXIT (1UL << 1)
	#define _SCR_SLEEPDEEP   (1UL << 2)
	#define _SCR_SEVONPEND   (1UL << 4)

	// flag-setting timeout, AKA PWR_FLAG_SETTING_DELAY_US (compared against SysTick ms tick, as HAL does)
	#define _PWR_FLAG_SETTING_DELAY 1000U

	void PWR_t::ConfigVoltageScaling(byte regulator) {
		PWR_D3CR_VOS = regulator;
		stduint tmpreg = PWR_D3CR_VOS;// Delay
	}

	void PWR_t::setMode(PWRMode mode, PWREntry entry, PWRRegulator regulator, PWRDomain domain) {
		switch (mode) {
		case PWRMode::SLEEP:
			_SCB_SCR() &= ~_SCR_SLEEPDEEP;
			if (entry == PWREntry::WFI) _ASM volatile("wfi");
			else { _ASM volatile("sev"); _ASM volatile("wfe"); _ASM volatile("wfe"); }
			break;
		case PWRMode::STOP:
			self[PWRReg::CR1].maset(0, 1, (stduint)regulator);// LPDS
			if (domain == PWRDomain::D1) {
				// system-level STOP: keep all domains in DSTOP, then enter deepsleep (HAL_PWR_EnterSTOPMode)
				self[PWRReg::CPUCR].rstof(0);// PDDS_D1
				self[PWRReg::CPUCR].rstof(1);// PDDS_D2
				self[PWRReg::CPUCR].rstof(2);// PDDS_D3
				_SCB_SCR() |= _SCR_SLEEPDEEP;
				_ASM volatile("dsb 0xF":::"memory");
				_ASM volatile("isb 0xF":::"memory");
				if (entry == PWREntry::WFI) _ASM volatile("wfi");
				else { _ASM volatile("sev"); _ASM volatile("wfe"); _ASM volatile("wfe"); }
				_SCB_SCR() &= ~_SCR_SLEEPDEEP;
			} else if (domain == PWRDomain::D2) self[PWRReg::CPUCR].rstof(1);// HAL_PWREx_EnterSTOPMode D2: clear PDDS_D2 only, no WFI
			else self[PWRReg::CPUCR].rstof(2);// HAL_PWREx_EnterSTOPMode D3: clear PDDS_D3 only, no WFI
			break;
		case PWRMode::STANDBY:
			if (domain == PWRDomain::D1) {
				// system-level STANDBY (HAL_PWR_EnterSTANDBYMode)
				self[PWRReg::CPUCR].setof(0, true);// PDDS_D1
				self[PWRReg::CPUCR].setof(1, true);// PDDS_D2
				self[PWRReg::CPUCR].setof(2, true);// PDDS_D3
				_SCB_SCR() |= _SCR_SLEEPDEEP;
				_ASM volatile("wfi");
			} else if (domain == PWRDomain::D2) self[PWRReg::CPUCR].setof(1, true);// HAL_PWREx_EnterSTANDBYMode D2: set PDDS_D2 only
			else self[PWRReg::CPUCR].setof(2, true);// HAL_PWREx_EnterSTANDBYMode D3: set PDDS_D3 only
			break;
		}
	}

	bool PWR_t::ControlVoltageScaling(PWRVoltageScale scaling) {
		self[PWRReg::D3CR].maset(14, 2, (stduint)scaling);// VOS
		uint64 tickstart = SysTick::getTick();
		while (!self[PWRReg::D3CR].bitof(13)) {// VOSRDY
			if ((SysTick::getTick() - tickstart) > _PWR_FLAG_SETTING_DELAY) return false;
		}
		return true;
	}

	stduint PWR_t::getVoltageRange() {
		return self[PWRReg::CSR1].mask(14, 2);// ACTVOS
	}

	void PWR_t::ControlStopModeVoltageScaling(PWRStopVoltageScale scaling) {
		self[PWRReg::CR1].maset(14, 2, (stduint)scaling);// SVOS
	}

	stduint PWR_t::getStopModeVoltageRange() {
		return self[PWRReg::CR1].mask(14, 2);// SVOS
	}

	bool PWR_t::ConfigSupply(stduint supply) {
#if defined(SMPS)
		self[PWRReg::CR3].maset(0, 6, supply);// SMPSLEVEL | SMPSEXTHP | SMPSEN | LDOEN | BYPASS
#else
		self[PWRReg::CR3].maset(0, 3, supply);// SCUEN | LDOEN | BYPASS
#endif
		uint64 tickstart = SysTick::getTick();
		while (!self[PWRReg::CSR1].bitof(13) && self[PWRReg::CR3].bitof(2)) {// ACTVOSRDY && SCUEN
			if ((SysTick::getTick() - tickstart) > _PWR_FLAG_SETTING_DELAY) return false;
		}
		return true;
	}

	stduint PWR_t::getSupplyConfig() {
#if defined(SMPS)
		return self[PWRReg::CR3].mask(0, 6);
#else
		return self[PWRReg::CR3].mask(0, 3);
#endif
	}

	void PWR_t::enBkUpAccess(bool ena) {
		self[PWRReg::CR1].setof(8, ena);// DBP
	}

	bool PWR_t::enBkUpReg(bool ena) {
		self[PWRReg::CR2].setof(0, ena);// BREN
		uint64 tickstart = SysTick::getTick();
		while (self[PWRReg::CR2].bitof(16) != ena) {// BRRDY
			if ((SysTick::getTick() - tickstart) > _PWR_FLAG_SETTING_DELAY) return false;
		}
		return true;
	}

	void PWR_t::enWakeUpPin(const PWRWakeUpPinConfig& cfg) {
		byte pin = (byte)cfg.pin;
		// WKUPEPR: WKUPEN at bit pin, WKUPP at bit 8+pin, WKUPPUPD at bits 16+2*pin
		self[PWRReg::WKUPEPR].maset(pin, 1, 1);
		self[PWRReg::WKUPEPR].maset(8 + pin, 1, cfg.polarity);
		self[PWRReg::WKUPEPR].maset(16 + 2 * pin, 2, cfg.pull);
		// EXTI_D1->IMR2: wakeup pins map to EXTI lines 55~60 (IMR2 bits 23~28), AKA HAL MODIFY_REG(EXTI_D1->IMR2, MASK, WakeUpPin<<23)
		EXTI[EXTICore::D1][EXTICoreReg::IMR2].maset(23, 6, (1U << pin));
	}

	stduint PWR_t::getWakeupFlag(stduint flag) {
		return self[PWRReg::WKUPFR].mask(0, 6) & flag;
	}

	bool PWR_t::clearWakeupFlag(stduint flag) {
		self[PWRReg::WKUPCR].maset(0, 6, flag);// WKUPCn: write 1 clears WKUPFn
		return (self[PWRReg::WKUPFR].mask(0, 6) & flag) == 0;
	}

	void PWR_t::ConfigProgrammableVoltageDetector(stduint level) {
		self[PWRReg::CR1].maset(5, 3, level);// PLS
	}

	void PWR_t::enProgrammableVoltageDetector(bool ena) {
		self[PWRReg::CR1].setof(4, ena);// PVDEN
	}

	void PWR_t::ConfigAnalogVoltageDetector(stduint level) {
		self[PWRReg::CR1].maset(17, 2, level);// ALS
	}

	void PWR_t::enAnalogVoltageDetector(bool ena) {
		self[PWRReg::CR1].setof(16, ena);// AVDEN
	}

	void PWR_t::enSleepOnExit(bool ena) {
		if (ena) _SCB_SCR() |= _SCR_SLEEPONEXIT; else _SCB_SCR() &= ~_SCR_SLEEPONEXIT;
	}

	void PWR_t::enSEVOnPend(bool ena) {
		if (ena) _SCB_SCR() |= _SCR_SEVONPEND; else _SCB_SCR() &= ~_SCR_SEVONPEND;
	}

	void PWR_t::ConfigD3Domain(stduint state) {
		self[PWRReg::CPUCR].setof(11, (bool)state);// RUN_D3
	}

	void PWR_t::enFlashPowerDown(bool ena) {
		self[PWRReg::CR1].setof(9, ena);// FLPS
	}

	bool PWR_t::enUSBReg(bool ena) {
		self[PWRReg::CR3].setof(25, ena);// USBREGEN
		uint64 tickstart = SysTick::getTick();
		while (self[PWRReg::CR3].bitof(26) != ena) {// USB33RDY
			if ((SysTick::getTick() - tickstart) > _PWR_FLAG_SETTING_DELAY) return false;
		}
		return true;
	}

	void PWR_t::enUSBVoltageDetector(bool ena) {
		self[PWRReg::CR3].setof(24, ena);// USB33DEN
	}

	void PWR_t::enBatteryCharging(bool ena, stduint resistor) {
		if (ena) self[PWRReg::CR3].setof(9, resistor != 0);// VBRS (HAL_PWREx_EnableBatteryCharging: set VBRS then VBE)
		self[PWRReg::CR3].setof(8, ena);// VBE (HAL_PWREx_DisableBatteryCharging: only clear VBE)
	}

	void PWR_t::enMonitoring(bool ena) {
		self[PWRReg::CR2].setof(4, ena);// MONEN
	}

	PWRMonitorLevel PWR_t::getTemperatureLevel() {
		stduint reg = self[PWRReg::CR2].mask(22, 2);// TEMPH | TEMPL
		if (reg == (1UL << 22)) return PWRMonitorLevel::BELOW_LOW;
		if (reg == (1UL << 23)) return PWRMonitorLevel::ABOVE_HIGH;
		return PWRMonitorLevel::BETWEEN;
	}

	PWRMonitorLevel PWR_t::getVBATLevel() {
		stduint reg = self[PWRReg::CR2].mask(20, 2);// VBATH | VBATL
		if (reg == (1UL << 20)) return PWRMonitorLevel::BELOW_LOW;
		if (reg == (1UL << 21)) return PWRMonitorLevel::ABOVE_HIGH;
		return PWRMonitorLevel::BETWEEN;
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

