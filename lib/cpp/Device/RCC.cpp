// UTF-8 CPP TAB4 CRLF
// Docutitle: Reset and Clock Control
// Datecheck: 20240420 ~ <Last-check>
// Developer: @dosconio
// Attribute: <ArnCovenant> <Environment> <Platform>
// Reference: <Reference>
// Dependens: <Dependence>
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
#include "../../../inc/cpp/Device/Flash"
#include "../../../inc/cpp/Device/SysTick"

namespace uni {
#if defined(_MCU_STM32F1x)
	bool RCCAHB::setMode(uint8 divexpo, bool usingPCLK1, bool usingPCLK2) {
		// "Set the highest APBx dividers in order to ensure that we do not go through a non-spec phase whatever we decrease or increase HCLK"
		Reference Cfgreg(_RCC_CFGR);
		if (usingPCLK1) Cfgreg |= 0x00000700;
		if (usingPCLK2) Cfgreg |= 0x00000700 << 3;
		// Set the new HCLK clock divider
		if (divexpo >= 9) return false;
		if (divexpo) divexpo = (divexpo - 1) | 0x8;// zero => zero-self
		Cfgreg |= divexpo << 4;// MGK number!
		// //{ERROR but no show out}
		return true;
	}
	//
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
#elif defined(_MCU_STM32F4x)

	Flash_t Flash;

	// naming from system_*chip*.c
	uint32_t SystemCoreClock = 16000000;
	const uint8_t AHBPrescTable[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9 };
	const uint8_t APBPrescTable[8] = { 0, 0, 0, 0, 1, 2, 3, 4 };
	/* default frequency
		HSE_VALUE    ((uint32_t)25000000)
		HSI_VALUE    ((uint32_t)16000000)
	*/

	bool RCC_t::setClock(SysclkSource::RCCSysclockSource source) {
		using namespace RCCReg;
		if (source == SysclkSource::HSE /*{TODO} || source == SysclkSource::HSI*/) {
			//{}bool use_HSE = true;
			//{TODO} RCC->CFGR = 0x00000000;// reset
			RCC[APB1ENR].setof(_RCC_APB1ENR_POSI_ENCLK_PWR);//aka __HAL_RCC_PWR_CLK_ENABLE
			//{} __HAL_PWR_VOLTAGESCALING_CONFIG // The voltage scaling allows optimizing the power consumption when the device is clocked below the maximum system frequency
			if (RCC.HSE.setMode() && RCC.PLL.setMode()); else
				return false;
			if (RCC.setFlash() && RCC.AHB.setMode() && RCC.Sysclock.setMode() && RCC.setFlash(true) && RCC.APB1.setMode(2) && RCC.APB2.setMode(1)); else
				return false;

			SystemCoreClock = RCC.Sysclock.getFrequency() >> AHBPrescTable[(RCC[CFGR] & _RCC_CFGR_MASK_HPRE) >> _RCC_CFGR_POSI_HPRE];
			SysTick::enClock(1000);// default 1000Hz

			Reference DBGMCU_IDCODE(0xE0042000UL /*DBGMCU_BASE+0x00*/);
			if ((DBGMCU_IDCODE >> 16) == 0x1001)// HAL_GetREVID() ぞ ((DBGMCU->IDCODE) >> 16U) よ if (HAL_GetREVID() == 0x1001) // STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported
				Flash[FlashReg::ACR].setof(_Flash_ACR_POSI_PRFTEN);//aka __HAL_FLASH_PREFETCH_BUFFER_ENABLE(); ぞ (FLASH->ACR |= FLASH_ACR_PRFTEN) // Enable the Flash prefetch
		}
		else return false;
		return true;
	}
	
	stduint RCC_t::getFrequencyPCLK1() {
		return (getFrequencyHCLK() >> APBPrescTable[(RCC[RCCReg::CFGR] & _RCC_CFGR_MASK_PPRE1) >> _RCC_CFGR_POSI_PPRE1]);
	}
	stduint RCC_t::getFrequencyPCLK2() {
		return (getFrequencyHCLK() >> APBPrescTable[(RCC[RCCReg::CFGR] & _RCC_CFGR_MASK_PPRE2) >> _RCC_CFGR_POSI_PPRE2]);
	}
	
	bool RCCSystemClock::setMode(SysclkSource::RCCSysclockSource source) {
		// if(((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_SYSCLK) == RCC_CLOCKTYPE_SYSCLK) then call this
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

	SysclkSource::RCCSysclockSource RCCSystemClock::getSource() {
		Reference Cfgreg = RCC[RCCReg::CFGR];
		return (SysclkSource::RCCSysclockSource)(Cfgreg & _RCC_CFGR_MASK_SCLKSWSource);
	}
	void RCCSystemClock::setSource(SysclkSource::RCCSysclockSource source) {
		Reference Cfgreg = RCC[RCCReg::CFGR];
		Cfgreg = (Cfgreg & ~_RCC_CFGR_MASK_Switch) | (((stduint)source << _RCC_CFGR_POSI_Switch) >> 2);
	}

	SysclkSource::RCCSysclockSource RCCSystemClock::CurrentSource() {
		return (SysclkSource::RCCSysclockSource)(RCC[RCCReg::CFGR] & _RCC_CFGR_MASK_SCLKSWSource);
	}

	stduint RCCSystemClock::getFrequency() {
		using namespace RCCReg;
		uint32_t pllm = 0U, pllvco = 0U, pllp = 0U;
		uint32_t sysclockfreq = 0U;

		switch (CurrentSource()) {
		case SysclkSource::HSE:
			sysclockfreq = 25000000U; // HSE_VALUE
			break;
		case SysclkSource::PLL:
			// PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLN
			// SYSCLK = PLL_VCO / PLLP
			pllm = RCC[PLLCFGR] & _RCC_PLLCFGR_MASK_PLLM;
			pllvco = (uint32_t)((((uint64_t)
				(/*__HAL_RCC_GET_PLL_OSCSOURCE()*/ RCC[PLLCFGR].bitof(_RCC_PLLCFGR_POSI_PLLSRC) ? 25000000U : 16000000U)
				* ((uint64_t)((RCC[PLLCFGR] & _RCC_PLLCFGR_MASK_PLLN) >> _RCC_PLLCFGR_POSI_PLLN)))) / (uint64_t)pllm);

			pllp = ((((RCC[PLLCFGR] & _RCC_PLLCFGR_MASK_PLLP) >> _RCC_PLLCFGR_POSI_PLLP) + 1U) * 2U);
			sysclockfreq = pllvco / pllp;
			break;
		case SysclkSource::HSI:
		default:
			sysclockfreq = 16000000U; // F4 HSI_VALUE
			break;
		}
		return sysclockfreq;
	}

	bool RCCOscillatorHSE::setMode() { //aka HAL_RCC_OscConfig
			//{TEMP} fixed parameters
			/*
			RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
			RCC_OscInitStruct.HSEState = RCC_HSE_ON;
			RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
			RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
			RCC_OscInitStruct.PLL.PLLM = 25;
			---- below (HSE) defaults are same HSI
			RCC_OscInitStruct.PLL.PLLN = 336;
			RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
			RCC_OscInitStruct.PLL.PLLQ = 7;
			RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
			RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
			RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1; // 2^0
			RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4; // 2^2
			RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; // 2^1
			*/
			HSEState::RCCOscillatorHSEState state = HSEState::Enable;
			//
			using namespace RCCReg;
			if ((RCCSystemClock::CurrentSource() == SysclkSource::HSE) || \
				(RCCSystemClock::CurrentSource() == SysclkSource::PLL) && \
				/*(RCCPLL::CurrentSource() == PLLSource::HSE)*/
				RCC[CFGR].bitof(_RCC_PLLCFGR_POSI_PLLSRC)) {
				//{TODO} ...
				return false;
			}
			//aka __HAL_RCC_HSE_CONFIG
			Reference Ctrlreg = RCC[CR];
			Ctrlreg &= ~HSEState::Bypass;
			switch (state) {
			case HSEState::Enable:
				Ctrlreg.setof(16, true); // RCC_CR_HSEON_Pos
				break;
			case HSEState::Bypass:
				Ctrlreg.setof(18, true); // RCC_CR_HSEBYP
				Ctrlreg.setof(16, true); // RCC_CR_HSEON_Pos
				break;
			default:
			case HSEState::Disable:
				Ctrlreg.setof(16, false); // RCC_CR_HSEON_Pos
				Ctrlreg.setof(18, false); // RCC_CR_HSEBYP
				break;
			}
			while ((state != HSEState::Disable) ^ !isReady());// wait, till HSE is bypassed or disabled while (ready zero)
			// PLL Config
			
			return true;
		}
	
		bool RCCOscillatorHSE::isReady() {
			return RCC[RCCReg::CR].bitof(_RCC_CR_POSI_HSEReady);
		}

		bool RCCOscillatorHSI::isReady() {
			return RCC[RCCReg::CR].bitof(_RCC_CR_POSI_HSIReady);
		}

		bool RCCPLL::isReady() {
			return RCC[RCCReg::CR].bitof(_RCC_CR_POSI_PLLReady);
		}

		void RCCPLL::enAble(bool ena) {
			Reference Conreg = RCC[RCCReg::CR];
			Conreg.setof(_RCC_CR_POSI_PLLEnable, ena);
			while (Conreg.bitof(_RCC_CR_POSI_PLLReady) ^ ena); // wait for PLL locked
		}
		
		bool RCCPLL::setMode() {
			// part PLL Configuration of HAL_RCC_OscConfig
			// if ((RCC_OscInitStruct->PLL.PLLState) != RCC_PLL_NONE) call this
			bool enabl_it = true;// if ((RCC_OscInitStruct->PLL.PLLState) == RCC_PLL_ON)
			if (RCC.Sysclock.CurrentSource() == SysclkSource::PLL) {
				return false;//{TEMP}
			}
			if (enabl_it)
			{
				enAble(false);
				stduint _m = 25;
				stduint _n = 336;
				stduint _p = 0x00000002U;//aka RCC_PLLP_DIV2
				stduint _q = 7;
				RCC[RCCReg::PLLCFGR] = (_TEMP PLLSource::HSE |
					(_m << _RCC_PLLCFGR_POSI_PLLM) |
					(_n << _RCC_PLLCFGR_POSI_PLLN) |
					(((_p >> 1U) - 1U) << _RCC_PLLCFGR_POSI_PLLP) |
					(_q << _RCC_PLLCFGR_POSI_PLLQ));
				enAble();
			}
			else
			{
				enAble(false);// aka __HAL_RCC_PLL_DISABLE();
				//{TODO} while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) != RESET);
			}
			return true;
		}

		// divexpo is a great design of the past me! --dosconio 20240717
		bool RCCAHB::setMode(_TEMP uint8 divexpo, bool usingPCLK1, bool usingPCLK2) {
			// if(((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_HCLK) == RCC_CLOCKTYPE_HCLK) call this
			// "Set the highest APBx dividers in order to ensure that we do not go through a non-spec phase whatever we decrease or increase HCLK"
			Reference Cfgreg = RCC[RCCReg::CFGR];
			if (usingPCLK1) Cfgreg |= 0x00001C00; //aka MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_HCLK_DIV16);
			if (usingPCLK2) Cfgreg |= 0x00001C00 << 3;
			// Set the new HCLK clock divider
			if (divexpo >= 9 _TODO) return false;
			if (divexpo) divexpo = (divexpo - 1) | 0x8;// zero => zero-self
			Cfgreg = (Cfgreg & ~(stduint)0x000000F0U) | (divexpo << 4);// MGK number!
			return true;
		}

		bool RCCAPB::setMode(uint8 divexpo) {
			Reference Cfgreg = RCC[RCCReg::CFGR];
			if (divexpo >= 5) return false;
			uint32 bitposi = 8 + PCLK_ID * 3;
			if (divexpo) divexpo = (divexpo - 1) | 0x4;
			Cfgreg = (Cfgreg & ~(uint32)(0x7 << bitposi)) | (divexpo << bitposi);
			return true;
		}



#endif
	//
	RCC_t RCC;
}

