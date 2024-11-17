// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Partial) RCC PLL Control
// Codifiers: @dosconio: 20241116
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

// inner included file ----

#if 0

#elif defined(_MCU_STM32F4x)

bool RCCPLL::isReady() {
	return RCC[RCCReg::CR].bitof(_RCC_CR_POSI_PLLReady);
}

void RCCPLL::enAble(bool ena) {
	Reference Conreg = RCC[RCCReg::CR];
	Conreg.setof(_RCC_CR_POSI_PLLEnable, ena);
	while (Conreg.bitof(_RCC_CR_POSI_PLLReady) ^ ena); // wait for PLL locked
}

bool RCCPLL::setMode() {
	using namespace RCCReg;
	// part PLL Configuration of HAL_RCC_OscConfig
	// if ((RCC_OscInitStruct->PLL.PLLState) != RCC_PLL_NONE) call this
	bool enabl_it = true;// if ((RCC_OscInitStruct->PLL.PLLState) == RCC_PLL_ON)
	if (RCC.Sysclock.CurrentSource() == SysclkSource::PLL) {
		return false;//{TEMP}
	}
	if (enabl_it)
	{
		enAble(false);
		stduint _m = 8;
		stduint _p = 0x00000002U;//aka RCC_PLLP_DIV2
		stduint _n = (168 / 8 * _p * _m);// = 336;
		stduint _q = 7;
		RCC[PLLCFGR] = (_TEMP PLLSource::HSE |
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

#elif defined(_MPU_STM32MP13)

stduint RCCPLL::CurrentSource() const {
	using namespace RCCReg;
	if (getID() == 1 || getID() == 2) {
		return RCC[RCK12SELR].masof(0, 2);// PLL12SRC
	}
	return nil;
}


stduint RCCPLL::getFrequencyP() const {
	using namespace RCCReg;
	stduint tmp = 0;
	if (getID() == 1 || getID() == 2) {
		float32 pll1m = RCC[PLL1CFGR1].masof(16, 6) + 1;// DIVM1
		bool pll1fracen = RCC[PLL1FRACR].bitof(16);// FRACLE
		float32 fracn1_us = pll1fracen ? RCC[PLL1FRACR].masof(3, 13) : 0;// FRACV
		float32 DIVN = RCC[PLL1CFGR1].masof(0, 9) + 1;
		float32 pll1vco = DIVN + (fracn1_us / (float32)0x2000);// Intermediary value
		switch (PLL1Source::PLL1SourceType(CurrentSource())) {
		case PLL1Source::HSI:
			pll1vco *= (float32)RCC._HSI_getFrequency() / pll1m;
			break;
		case PLL1Source::HSE:
			pll1vco *= (float32)HSE_VALUE / pll1m;
			break;
		case PLL1Source::OFF: default:
			pll1vco = 0;
			break;
		}
		const stduint P_Div = RCC[PLL1CFGR2].masof(0, 7) + 1;
		return pll1vco / (float32)P_Div;// aka PLL1/2_Clocks->PLL1_P/Q/R_Frequency
	}
	return 0;
}

#endif
