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

namespace uni {

	// HSE setMode
#ifdef _MCU_STM32F1x
	bool RCCOscillatorHSE::setMode(HSEState::RCCOscillatorHSEState state, uint32 predivr) {
		using namespace RCCReg;
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
#endif
}

