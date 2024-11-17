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

#include "../../../inc/cpp/Device/RCC/RCC"
#include "../../../inc/cpp/Device/Flash"
#include "../../../inc/cpp/Device/SysTick"

//{TODO} User Can Def by Macro
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_CW32F030)
// : default frequency
stduint HSE_VALUE = (8000000);
stduint HSI_VALUE = (8000000);
#elif defined(_MPU_STM32MP13)
stduint HSE_VALUE = (24000000U);
stduint HSI_VALUE = (64000000U);
#endif


namespace uni {
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
	const uint8_t AHBPrescTable[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9 };
	const uint8_t APBPrescTable[8] = { 0, 0, 0, 0, 1, 2, 3, 4 };

	stduint RCC_t::getFrequencyPCLK1() {
		using namespace RCCReg;
		return (getFrequencyHCLK() >> APBPrescTable[(RCC[CFGR] & _RCC_CFGR_MASK_PPRE1) >> _RCC_CFGR_POSI_PPRE1]);
	}
	stduint RCC_t::getFrequencyPCLK2() {
		using namespace RCCReg;
		return (getFrequencyHCLK() >> APBPrescTable[(RCC[CFGR] & _RCC_CFGR_MASK_PPRE2) >> _RCC_CFGR_POSI_PPRE2]);
	}
#endif

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
	
	
#elif defined(_MCU_STM32F4x)

	Flash_t Flash;

	bool RCCOscillatorHSE::isReady() {
		return RCC[RCCReg::CR].bitof(_RCC_CR_POSI_HSEReady);
	}

	bool RCCOscillatorHSI::isReady() {
		return RCC[RCCReg::CR].bitof(_RCC_CR_POSI_HSIReady);
	}

	static byte _tab_AHB_PRES_EXPO[] = {
		0,  8,  9, 10, 11,  0, 12, 13, 14, 15
	};
	// divexpo is a great design of the past me! --dosconio 20240717
	bool RCCAHB::setMode(_TEMP uint8 divexpo, bool usingPCLK1, bool usingPCLK2) {
		using namespace RCCReg;
		// if(((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_HCLK) == RCC_CLOCKTYPE_HCLK) call this
		// "Set the highest APBx dividers in order to ensure that we do not go through a non-spec phase whatever we decrease or increase HCLK"
		if (divexpo >= numsof(_tab_AHB_PRES_EXPO)) return false;
		Reference Cfgreg = RCC[CFGR];
		if (usingPCLK1) Cfgreg |= 0x00001C00; //aka MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_HCLK_DIV16);
		if (usingPCLK2) Cfgreg |= 0x00001C00 << 3;
		// Set the new HCLK clock divider
		Cfgreg.maset(4, 4, _tab_AHB_PRES_EXPO[divexpo]);// HPRE
		return true;
	}

	// F4 diff with F1
	bool RCCAPB::setMode(uint8 divexpo) {
		using namespace RCCReg;
		Reference Cfgreg = RCC[CFGR];
		if (divexpo >= 5) return false;
		uint32 bitposi = 10 + PCLK_ID * 3;// PCLK_ID is 0 or 1
		if (divexpo) divexpo = (divexpo - 1) | 0x4;
		Cfgreg = (Cfgreg & ~(uint32)(0x7 << bitposi)) | (divexpo << bitposi);
		return true;
	}

#elif defined(_MPU_STM32MP13)
	stduint RCC_t::_HSI_getFrequency() const {
		using namespace RCCReg;
		stduint tmp = 0;
		if (RCC[OCRDYR].bitof(_RCC_OCRDYR_POS_HSIDIVRDY))
			tmp = RCC[HSICFGR].masof(0, 2);// HSIDIV aka __HAL_RCC_GET_HSI_DIV{DIV 1,2,4,8}
		return HSI_VALUE >> tmp;
	}

#endif

	#include "../../../inc/c/driver/RCC/RCC-setClock.hpp"
	#include "../../../inc/c/driver/RCC/PLL.hpp"
		
	#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
		RCC_t RCC;
	#endif
}

