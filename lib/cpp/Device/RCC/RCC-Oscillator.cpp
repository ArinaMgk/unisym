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
#endif
}

