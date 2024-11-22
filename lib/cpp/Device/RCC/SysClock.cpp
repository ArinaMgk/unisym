// UTF-8  CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) Reset and Clock Control
// Codifiers: @dosconio: 20240420 ~ <Last-check> 
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

#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/c/driver/RCC/RCC-registers.hpp"
#include "../../../../inc/cpp/Device/Flash"
#include "../../../../inc/cpp/Device/SysTick"

extern stduint HSE_VALUE;
extern stduint HSI_VALUE;

stduint SystemCoreClock = _SCC_DEFAULT;
namespace uni {
	// ---- setMode ----
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
	// [F4] if(((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_SYSCLK) == RCC_CLOCKTYPE_SYSCLK) then call this
	bool RCCSystemClock::setMode(SysclkSource::RCCSysclockSource source) {
		bool state;
		switch (source) {
		case SysclkSource::HSI:
			state = RCCOscillatorHSI::isReady();
			break;
		case SysclkSource::HSE:
			state = RCCOscillatorHSE::isReady();
			break;
		case SysclkSource::PLL:
			state = RCCPLL::isReady();
			break;
		default:
			state = false;
			break;
		}
		if (!state) return false;
		setSource(source);
		while (getSource() != source);
		return true;
	}
#elif defined(_MPU_STM32MP13)
	bool RCCSystemClock::setMode(SysclkSource::RCCSysclockSource source, byte divexpo) {
		//: Ensure clock source is ready
		switch (source) {
		case SysclkSource::HSI:
			if (!RCC.HSI.isReady()) return false;
			break;
		case SysclkSource::HSE:
			if (!RCC.HSE.isReady()) return false;
			break;
		case SysclkSource::PLL1:
			if (!RCC.PLL1.isReady()) return false;
			break;
		case SysclkSource::MPUDIV:
			if (divexpo > 4) return false;
			if (!RCC.PLL1.isReady()) return false;
			setDiv(divexpo);
			break;
		default: return false;
		}
		setSource(source);
		while (true != isReady());
		//: [Cortex A7]
		RCC.Sysclock.getCoreFrequency();
		return SysTick::enClock(SysTickHz);
	}
#endif
	// ---- xetSource ----
#if 0
#elif defined(_MCU_STM32F4x)
	SysclkSource::RCCSysclockSource RCCSystemClock::getSource() {
		Reference Cfgreg = RCC[RCCReg::CFGR];
		return (SysclkSource::RCCSysclockSource)(Cfgreg & _RCC_CFGR_MASK_SCLKSWSource);
	}
	void RCCSystemClock::setSource(SysclkSource::RCCSysclockSource source) {
		Reference Cfgreg = RCC[RCCReg::CFGR];
		Cfgreg = (Cfgreg & ~_RCC_CFGR_MASK_Switch) | ((_IMM(source) << _RCC_CFGR_POSI_Switch) >> 2);
	}
#elif defined(_MPU_STM32MP13)
	void RCCSystemClock::setSource(SysclkSource::RCCSysclockSource source) {
		RCC[RCCReg::MPCKSELR].maset(0, 2, _IMM(source));
	}
#endif
	// ---- getFrequency (SYSCLK) ----
#if defined(_MCU_STM32F1x)
//{TEMP} only for F103
	stduint RCCSystemClock::getFrequency() {
		// If SYSCLK source is PLL, function returns a value based on HSE_VALUE 		divided by PREDIV factor(**) or HSI_VALUE(*) multiplied by the PLL factor.
		using namespace RCCReg;
		//{TODO} RCC_CFGR2_PREDIV1SRC
		//{TODO} RCC_CFGR2_PREDIV1
		uint32_t pllm = 0U,// PLLMUL factor
			pllclk = 0U, prediv = 0U;
		uint32_t sysclockfreq = 0U;

		switch (CurrentSource()) {
		case SysclkSource::HSE:
			sysclockfreq = HSE_VALUE;
			break;
		case SysclkSource::PLL:
			const static byte aPLLMULFactorTable[16] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 16 };
			const static byte aPredivFactorTable[2] = { 1, 2 };
			pllm = aPLLMULFactorTable[_IMM(RCC[CFGR] & _RCC_CFGR_MASK_PLLMUL) >> _RCC_CFGR_POSI_PLLMUL];
			if ((RCC[CFGR] & _RCC_CFGR_MASK_PLLSource) != 0 /*RCC_PLLSOURCE_HSI_DIV2*/) {
				prediv = aPredivFactorTable[_IMM(RCC[CFGR] & (1 << _RCC_CFGR_POSI_PLL_XTPRE)) >> _RCC_CFGR_POSI_PLL_XTPRE];
				// HSE used as PLL clock source : PLLCLK = HSE/PREDIV1 * PLLMUL
				pllclk = ((HSE_VALUE * pllm) / prediv);
			}
			else {
				// HSI used as PLL clock source : PLLCLK = HSI/2 * PLLMUL
				pllclk = ((HSI_VALUE >> 1) * pllm);
			}
			sysclockfreq = pllclk;
			break;
		case SysclkSource::HSI:
		default:
			sysclockfreq = HSI_VALUE;
			break;
		}
		return sysclockfreq;
	}
#elif defined(_MCU_STM32F4x)
	stduint RCCSystemClock::getFrequency() {
		using namespace RCCReg;
		uint32_t pllm = 0U, pllvco = 0U, pllp = 0U;
		uint32_t sysclockfreq = 0U;

		switch (CurrentSource()) {
		case SysclkSource::HSE:
			sysclockfreq = HSE_VALUE;
			break;
		case SysclkSource::PLL:
			// PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLN
			// SYSCLK = PLL_VCO / PLLP
			pllm = RCC[PLLCFGR] & _RCC_PLLCFGR_MASK_PLLM;
			pllvco = (uint32_t)((((uint64_t)
				(/*__HAL_RCC_GET_PLL_OSCSOURCE()*/ RCC[PLLCFGR].bitof(_RCC_PLLCFGR_POSI_PLLSRC) ? HSE_VALUE : HSI_VALUE)
				* ((uint64_t)((RCC[PLLCFGR] & _RCC_PLLCFGR_MASK_PLLN) >> _RCC_PLLCFGR_POSI_PLLN)))) / (uint64_t)pllm);

			pllp = ((((RCC[PLLCFGR] & _RCC_PLLCFGR_MASK_PLLP) >> _RCC_PLLCFGR_POSI_PLLP) + 1U) * 2U);
			sysclockfreq = pllvco / pllp;
			break;
		case SysclkSource::HSI:
		default:
			sysclockfreq = HSI_VALUE;
			break;
		}
		return sysclockfreq;
	}
#endif
	// ---- getFrequency (HCLK) ----
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
	extern const uint8_t AHBPrescTable[];
	stduint RCCSystemClock::getCoreFrequency() {
		using namespace RCCReg;
		const stduint AHB_Prescaler = RCC[CFGR].masof(_RCC_CFGR_POSI_HPRE, _RCC_CFGR_LENG_HPRE);
		return SystemCoreClock = RCC.Sysclock.getFrequency() >> AHBPrescTable[AHB_Prescaler];
	}
#elif defined(_MPU_STM32MP13)
	stduint RCCSystemClock::getCoreFrequency() {
		using namespace RCCReg;
		stduint mpu_div;
		switch (CurrentSource()) {
		case SysclkSource::HSI:
			SystemCoreClock = RCC.HSI.getFrequency();
			break;
		case SysclkSource::HSE:
			SystemCoreClock = HSE_VALUE;
			break;
		case SysclkSource::PLL1:
			SystemCoreClock = RCC.PLL1.getFrequencyP();
			break;
		case SysclkSource::MPUDIV:
			mpu_div = RCC.getMPUDiv();
			SystemCoreClock = mpu_div ? RCC.PLL1.getFrequencyP() / mpu_div : 0;
			break;
		default: return 0;
		}
		return SystemCoreClock;
	}
#endif
	// ---- CurrentSource ---- {TODO} comb F1&F4 and use masof
#if defined(_MCU_STM32F1x)
	SysclkSource::RCCSysclockSource RCCSystemClock::CurrentSource() {
		Reference _RCC_ConfigRegister(_RCC_CFGR);
		return (SysclkSource::RCCSysclockSource)(_RCC_ConfigRegister & _RCC_CFGR_MASK_SCLKSWSource);
	}
#elif defined(_MCU_STM32F4x)
	SysclkSource::RCCSysclockSource RCCSystemClock::CurrentSource() {
		return (SysclkSource::RCCSysclockSource)(RCC[RCCReg::CFGR] & _RCC_CFGR_MASK_SCLKSWSource);
	}
#elif defined(_MPU_STM32MP13)
	SysclkSource::RCCSysclockSource RCCSystemClock::CurrentSource() {
		return SysclkSource::RCCSysclockSource(RCC[RCCReg::MPCKSELR] & 0b11);
	}
#endif
	// ---- isReady ----
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
#elif defined(_MPU_STM32MP13)
	bool RCCSystemClock::isReady() {
		return RCC[RCCReg::MPCKSELR].bitof(31);
	}
#endif
	// ---- setDiv ----
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
#elif defined(_MPU_STM32MP13)
	void RCCSystemClock::setDiv(byte divexpo) {
		using namespace RCCReg;
		RCC[MPCKDIVR].maset(_IMM(_MPCKDIVR::MPUDIV), 4, divexpo);
		// need wait until ready ?
	}
#endif
	
}
