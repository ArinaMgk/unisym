// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device/Compnent) RCC-Oscillator
// Codifiers: @dosconio: 20241028
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
#include "../../../../inc/cpp/Device/RCC/RCCClock"
#include "../../../../inc/cpp/Device/SysTick"
#include "../../../../inc/cpp/Device/Power"
#include "../../../../inc/c/driver/RCC/RCC-registers.hpp"


extern stduint HSE_VALUE;
extern stduint HSI_VALUE;

namespace uni {
	using namespace RCCReg;
	// HSE setMode
#ifdef _MCU_STM32F1x
	bool RCCOscillatorHSE::setMode(HSEState::RCCOscillatorHSEState state, uint32 predivr) {
		if ((RCCSystemClock::CurrentSource() == SysclkSource::HSE) || \
			(RCCSystemClock::CurrentSource() == SysclkSource::PLL) && \
			(RCCPLL::CurrentSource() == PLLSource::HSE)) { // Condition of disable
			//{TODO} Cancel Sysclock: if((__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) != RESET) && (RCC_OscInitStruct->HSEState == RCC_HSE_OFF))
			return false;// it is being used
			//if (!Reference(_RCC_CR).bitof(_RCC_CR_POSI_HSEReady))
			//	return false;
		}
		else {
			switch (state) {
			case HSEState::Enable:
				RCC[CR].setof(16);// HSEON
				break;
			case HSEState::Bypass:
				RCC[CR].setof(18);// HSEBYP
				RCC[CR].setof(16);// HSEON
				break;
			default:
			case HSEState::Disable:
				RCC[CR].setof(16, false);// HSEON
				RCC[CR].setof(18, false);// HSEBYP
				break;
			}
			//
			while (!isReady());//{tofix}
			//{TODO} Check the HSE State by ticks
			//{TODO} Tick Delay
			this->predivr = predivr;
			return true;
		}
	}
#elif defined(_MCU_STM32F4x)
	// F407VET: SYSCLK=HSx/M*N/P
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
#elif defined(_MPU_STM32MP13)

	// ---- isReady
	bool RCCOscillatorHSE::isReady() const {
		return RCC[OCRDYR].bitof(_OCRDYR::HSERDY);
	}
	bool RCCOscillatorHSI::isReady() const {
		return RCC[OCRDYR].bitof(_OCRDYR::HSIRDY);
	}
	bool RCCOscillatorCSI::isReady() const {
		return RCC[OCRDYR].bitof(_OCRDYR::CSIRDY);
	}
	bool RCCOscillatorLSI::isReady() const {
		return RCC[RDLSICR].bitof(_IMM(_RDLSICR::LSIRDY));
	}
	bool RCCOscillatorLSE::isReady() const {
		return RCC[BDCR].bitof(_IMM(_BDCR::LSERDY));
	}
	// ---- enAble
	void RCCOscillatorHSE::enAble(bool ena) const {
		RCC[ena ? OCENSETR : OCENCLRR] = _IMM1S(_OCENSETR::HSEON);// not setof
	}
	void RCCOscillatorHSI::enAble(bool ena) const {
		RCC[ena ? OCENSETR : OCENCLRR] = _IMM1S(_OCENSETR::HSION);
	}
	void RCCOscillatorCSI::enAble(bool ena) const {
		RCC[ena ? OCENSETR : OCENCLRR] = _IMM1S(_OCENSETR::CSION);
	}
	void RCCOscillatorLSI::enAble(bool ena) const {
		RCC[RDLSICR].setof(_IMM(_RDLSICR::LSION), ena);
	}
	void RCCOscillatorLSE::enAble(bool ena) const {
		RCC[BDCR].setof(_IMM(_BDCR::LSEON), ena);
	}
	// ---- isUsed     {uchk}
	static bool isUsed_for_PLL(bool hse) {
		// in fact, all HSI zero and HSE one
		bool pll12_used = RCC.PLL1.CurrentSource() ==
			_IMM(hse ? PLL1Source::HSE : PLL1Source::HSI);
		asserv(pll12_used) = RCC.PLL1.isReady() && RCC.PLL2.isReady();
		bool pll3_used = (RCC.PLL3.CurrentSource() ==
			_IMM(hse ? PLL3Source::HSE : PLL3Source::HSI))
			&& RCC.PLL3.isReady();
		bool pll4_used = (RCC.PLL4.CurrentSource() ==
			_IMM(hse ? PLL4Source::HSE : PLL4Source::HSI))
			&& RCC.PLL4.isReady();
		return pll12_used || pll3_used || pll4_used;
	}
	bool RCCOscillatorHSE::isUsed() const {
		bool sclk_used = RCCSystemClock::CurrentSource() == SysclkSource::HSE;
		asserv(sclk_used) = RCCSystemClock::isReady();
		if (sclk_used) return true;
		//
		bool axis_used = RCC.AKA__HAL_RCC_GET_AXIS_SOURCE() == AxisSource::HSE;
		asserv(axis_used) = RCC.AKA_RCC_FLAG_AXISSRCRDY();
		if (axis_used) return true;
		//
		bool mlahb_used = RCC.AKA__HAL_RCC_GET_MLAHB_SOURCE() == MLAHBSource::HSE;
		asserv(mlahb_used) = RCC.AKA_RCC_FLAG_MLAHBSSRCRDY();
		if (mlahb_used) return true;
		return isUsed_for_PLL(true);
	}
	bool RCCOscillatorHSI::isUsed() const {
		bool sclk_used = RCCSystemClock::CurrentSource() == SysclkSource::HSI;
		asserv(sclk_used) = RCCSystemClock::isReady();
		if (sclk_used) return true;
		//
		bool axis_used = RCC.AKA__HAL_RCC_GET_AXIS_SOURCE() == AxisSource::HSI;
		asserv(axis_used) = RCC.AKA_RCC_FLAG_AXISSRCRDY();
		if (axis_used) return true;
		//
		bool mlahb_used = RCC.AKA__HAL_RCC_GET_MLAHB_SOURCE() == MLAHBSource::HSI;
		asserv(mlahb_used) = RCC.AKA_RCC_FLAG_MLAHBSSRCRDY();
		if (mlahb_used) return true;
		return isUsed_for_PLL(false);
	}
	bool RCCOscillatorCSI::isUsed() const {
		bool mlahb_used = RCC.AKA__HAL_RCC_GET_MLAHB_SOURCE() == MLAHBSource::CSI;
		asserv(mlahb_used) = RCC.AKA_RCC_FLAG_MLAHBSSRCRDY();
		if (mlahb_used) return true;
		bool pll3_used = (RCC.PLL3.CurrentSource() == _IMM(PLL3Source::CSI))
			&& RCC.PLL3.isReady();
		bool pll4_used = (RCC.PLL4.CurrentSource() == _IMM(PLL4Source::CSI))
			&& RCC.PLL4.isReady();
		return pll3_used || pll4_used;
	}
	// ---- State not-OFF
	bool RCCOscillatorHSE::setMode(bool bypass, bool byp_dig) const {
		// RCC_HSE_ON = TRUE
		if (isUsed()) return true;
		enAble(false); while (false != isReady());
		RCC[OCENCLRR] = _IMM1S(_OCENCLRR::DIGBYP) | _IMM1S(_OCENSETR::HSEBYP);
		if (bypass) {
			if (byp_dig)
				RCC[OCENSETR].setof(_OCENSETR::DIGBYP);
			RCC[OCENSETR].setof(_OCENSETR::HSEBYP);
		}
		enAble(true); while (true != isReady());
		return true;
	}
	bool RCCOscillatorHSI::setMode(byte divexpo, byte calibration) const {
		if (isUsed()) {
			AdjectCalibration(calibration);
			// No change HSI if used for PLL
			if (!isUsed_for_PLL(false)) {
				setDiv(divexpo);
			}
			RCC.Sysclock.getCoreFrequency();
			return SysTick::enClock(1000);
		}
		else {
			enAble(true); while (true != isReady());
			setDiv(divexpo);
			AdjectCalibration(calibration);
		}
		return true;
	}
	bool RCCOscillatorCSI::setMode(byte calibration) const {
		if (!isUsed()) {
			enAble(true); while (true != isReady());
		}
		AdjectCalibration(calibration);
		return true;
	}
	bool RCCOscillatorLSI::setMode() const {
		enAble(true); while (true != isReady());
		return true;
	}
	bool RCCOscillatorLSE::setMode(bool bypass, bool byp_dig) const {
		PWR.setDBP(true);// Enable access to RTC and backup registers
		// Beware LSE oscillator is not used as clock before using this function.
		// As the LSE is in the Backup domain and write access is denied to this domain after reset, you have to enable write access using HAL_PWR_EnableBkUpAccess() function (set PWR_CR1_DBP bit) before to configure the LSE (to be done once after reset).
		canMode();
		if (bypass) {
			if (byp_dig)
				RCC[BDCR].setof(_IMM(_BDCR::DIGBYP), true);
			RCC[BDCR].setof(_IMM(_BDCR::LSEBYP), true);
		}
		enAble(true); while (true != isReady());
		return true;
	}
	// ---- State OFF
	bool RCCOscillatorHSE::canMode() const {
		// RCC_HSE_ON = FALSE
		if (isUsed()) return false;
		enAble(false); while (false != isReady());
		RCC[OCENCLRR] = _IMM1S(_OCENCLRR::DIGBYP) | _IMM1S(_OCENSETR::HSEBYP);
		return true;
	}
	bool RCCOscillatorHSI::canMode() const {
		// Drop usage before canMode
		if (isUsed()) return false;
		enAble(false); while (false != isReady());
		return true;
	}
	bool RCCOscillatorCSI::canMode() const {
		if (isUsed()) return false;
		enAble(false); while (false != isReady());
		return true;
	}
	bool RCCOscillatorLSI::canMode() const {
		enAble(false); while (false != isReady());
		return true;
	}
	bool RCCOscillatorLSE::canMode() const {
		enAble(false); while (false != isReady());
		RCC[BDCR].setof(_IMM(_BDCR::LSEBYP), false);
		RCC[BDCR].setof(_IMM(_BDCR::DIGBYP), false);
		return true;
	}














	
	void RCCOscillatorHSI::Reset() const {
		RCC[HSICFGR].maset(_HSICFGR::HSIDIV, 2, 0);
		while (!isReady());
		RCC[HSICFGR].maset(_HSICFGR::HSITRIM, 7, 0);
	}

	stduint RCCOscillatorHSI::getFrequency() const {
		stduint tmp = 0;
		if (RCC[OCRDYR].bitof(_OCRDYR::HSIDIVRDY))
			tmp = RCC[HSICFGR].masof(0, 2);// HSIDIV aka __HAL_RCC_GET_HSI_DIV{DIV 1,2,4,8}
		return HSI_VALUE >> tmp;
	}



	void RCCOscillatorHSI::setDiv(byte div) const {
		RCC[HSICFGR].maset(_HSICFGR::HSIDIV, 2, div);
		while (!RCC[OCRDYR].bitof(_OCRDYR::HSIDIVRDY));
	}
	void RCCOscillatorHSI::AdjectCalibration(stduint calibration) const {
		RCC[HSICFGR].maset(_HSICFGR::HSITRIM, 7, calibration);
	}
	void RCCOscillatorCSI::AdjectCalibration(stduint calibration) const {
		RCC[CSICFGR].maset(_IMM(_CSICFGR::CSITRIM), 5, calibration);
	}

#endif
}

