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

#define _MCU_RCC_TEMP
#define _MCU_PWR_TEMP
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/Device/RCC/RCCClock"
#include "../../../../inc/cpp/Device/SysTick"
#include "../../../../inc/cpp/Device/_Power.hpp"
#include "../../../../inc/c/driver/RCC/RCC-registers.hpp"


extern stduint HSE_VALUE;
extern stduint HSI_VALUE;

namespace uni {
	

// isReady

#if 0
#elif defined(_MCU_STM32H7x)
	bool RCCOscillatorHSI::isReady() const {
		return RCC[RCCReg::CR].bitof(2);
	}
	bool RCCOscillatorHSE::isReady() const {
		return RCC[RCCReg::CR].bitof(17);
	}
	bool RCCOscillatorHSI48::isReady() const {
		return RCC[RCCReg::CR].bitof(13);
	}
	bool RCCOscillatorCSI::isReady() const { // Internal RC 4MHz oscillator clock
		return RCC[RCCReg::CR].bitof(8);
	}
	bool RCCOscillatorLSI::isReady() const {
		return RCC[RCCReg::CSR].bitof(1);
	}
	bool RCCOscillatorLSE::isReady() const {
		return RCC[RCCReg::BDCR].bitof(1);
	}
#elif defined(_MPU_STM32MP13)
	using namespace RCCReg;

	bool RCCOscillatorHSI::isReady() const {
		return RCC[OCRDYR].bitof(_OCRDYR::HSIRDY);
	}
	bool RCCOscillatorHSE::isReady() const {
		return RCC[OCRDYR].bitof(_OCRDYR::HSERDY);
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
#endif




	// HSE setMode
#ifdef _MCU_STM32F1x
	using namespace RCCReg;

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
	using namespace RCCReg;

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
#elif defined(_MCU_STM32H7x)

#include "../../../../inc/cpp/Device/_Power.hpp"


	// ---- setMode


	// HSI
	
	stduint RCCOscillatorHSI::getFrequency_ToCore() const {
		Stdfield HSIDIV(RCC[RCCReg::CR], 3, 2);
		return HSI_VALUE >> _IMM(HSIDIV);
	}

	bool RCCOscillatorHSI::setMode(bool ena, byte divr, stduint calibration) const {
		asrtret(divr < 4 && calibration <= 0xFFF);
		/* When the HSI is used as system clock it will not disabled */
		if ((RCCSystemClock::CurrentSource() == SysclkSource::HSI) ||
			((RCCSystemClock::CurrentSource() == SysclkSource::PLL1) && ((PLLSource::PLLSource)_IMM(RCC_PLLCKSELR_PLLSRC) == PLLSource::HSI)))
		{
			asrtret (isReady() && !ena) else
			{
				Reflocal(cr) = RCC[RCCReg::CR];
				cr.setof(0, ena); // RCC_CR_HSION
				cr.maset(3, 2, divr); // RCC_CR_HSIDIV
				RCC[RCCReg::CR] = cr;
				while (!ena ^ isReady());
				RCC_ICSCR_HSITRIM = calibration;// __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST
			}
		}
		else {
			if (ena) {
				Reflocal(cr) = RCC[RCCReg::CR];
				cr.setof(0, ena); // RCC_CR_HSION
				cr.maset(3, 2, divr); // RCC_CR_HSIDIV
				RCC[RCCReg::CR] = cr;
				while (!ena ^ isReady());
				RCC_ICSCR_HSITRIM = calibration;// __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST
			}
			else {
				RCC[RCCReg::CR].setof(0, false); // RCC_CR_HSION
				while (isReady());
			}
		}
		return true;
	}

	bool RCCOscillatorHSI48::setMode(bool ena) const {
		RCC[RCCReg::CR].setof(12, ena); // RCC_CR_HSI48ON
		while (ena != isReady());
		return true;
	}

	// HSE

	bool RCCOscillatorHSE::setMode(bool ena, bool bypass) const {
		if (!ena) bypass = false;
		// When the HSE is used as system clock or clock source for PLL in these cases HSE will not disabled 
		if ((RCCSystemClock::CurrentSource() == SysclkSource::HSE) ||
			((RCCSystemClock::CurrentSource() == SysclkSource::PLL1) && ((PLLSource::PLLSource)_IMM(RCC_PLLCKSELR_PLLSRC) == PLLSource::HSE)))
		{
			if (isReady() && !ena) return false;
		}
		else {
			// __HAL_RCC_HSE_CONFIG
			RCC[RCCReg::CR].setof(16, ena);// RCC_CR_HSEON
			RCC[RCCReg::CR].setof(18, bypass);// RCC_CR_HSEBYP
			while (ena != isReady());
		}
		return true;
	}

	// CSI
	void RCCOscillatorCSI::enAble(bool ena) const {
			RCC[RCCReg::CR].setof(7, ena);// RCC_CR_CSION
			while (ena != isReady());
	}
	bool RCCOscillatorCSI::setMode(bool ena, stduint calibration) const {
		asrtret(calibration <= 0x1F);
		if ((RCCSystemClock::CurrentSource() == SysclkSource::CSI) ||
			((RCCSystemClock::CurrentSource() == SysclkSource::PLL1) && ((PLLSource::PLLSource)_IMM(RCC_PLLCKSELR_PLLSRC) == PLLSource::CSI)))
		{
			asrtret(isReady() && !ena) else {
				RCC_ICSCR_CSITRIM = calibration;// __HAL_RCC_CSI_CALIBRATIONVALUE_ADJUST
			}
		}
		else {
			enAble(ena);
			if (ena) {
				RCC_ICSCR_CSITRIM = calibration;// __HAL_RCC_CSI_CALIBRATIONVALUE_ADJUST
			}
		}
		return true;
	}

	// LSE

	bool RCCOscillatorLSE::setMode(bool ena, bool bypass) const {
		if (!ena) bypass = false;
		// Enable write access to Backup domain
		PWR[PWRReg::CR1] |= PWR_CR1_DBP;
		while (PWR[PWRReg::CR1] & PWR_CR1_DBP);
		// __HAL_RCC_LSE_CONFIG
		RCC[RCCReg::BDCR].setof(0, ena);// RCC_BDCR_LSEON
		RCC[RCCReg::BDCR].setof(2, bypass);// RCC_BDCR_LSEBYP
		while (ena != isReady());
		return true;
	}

	// LSI

	bool RCCOscillatorLSI::setMode(bool ena) const {
		RCC[RCCReg::CSR].setof(0, ena); // RCC_CSR_LSION
		while (ena != isReady());
		return true;
	}


#elif defined(_MPU_STM32MP13)
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
		bool axis_used = RCC.AXIS.getSource() == AxisSource::HSE;
		asserv(axis_used) = RCC.AXIS.isReady();
		if (axis_used) return true;
		//
		bool mlahb_used = RCC.MLAHB.getSource() == MLAHBSource::HSE;
		asserv(mlahb_used) = RCC.MLAHB.isReady();
		if (mlahb_used) return true;
		return isUsed_for_PLL(true);
	}
	bool RCCOscillatorHSI::isUsed() const {
		bool sclk_used = RCCSystemClock::CurrentSource() == SysclkSource::HSI;
		asserv(sclk_used) = RCCSystemClock::isReady();
		if (sclk_used) return true;
		//
		bool axis_used = RCC.AXIS.getSource() == AxisSource::HSI;
		asserv(axis_used) = RCC.AXIS.isReady();
		if (axis_used) return true;
		//
		bool mlahb_used = RCC.MLAHB.getSource() == MLAHBSource::HSI;
		asserv(mlahb_used) = RCC.MLAHB.isReady();
		if (mlahb_used) return true;
		return isUsed_for_PLL(false);
	}
	bool RCCOscillatorCSI::isUsed() const {
		bool mlahb_used = RCC.MLAHB.getSource() == MLAHBSource::CSI;
		asserv(mlahb_used) = RCC.MLAHB.isReady();
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
			return SysTick::enClock(SysTickHz);
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

