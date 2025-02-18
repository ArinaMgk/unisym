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
#include "../../../inc/c/driver/RCC/RCC-registers.hpp"

// high/low/(C) speed external/internal oscillator
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_CW32F030)
// : default frequency
extern stduint HSE_VALUE, HSI_VALUE;
stduint HSE_VALUE = (8000000);
stduint HSI_VALUE = (8000000);
#elif defined(_MCU_STM32H7x)
stduint HSE_VALUE = (25000000);
stduint HSI_VALUE = (64000000);
stduint LSE_VALUE = (32768);
stduint CSI_VALUE = (4000000);

#elif defined(_MPU_STM32MP13)
stduint HSE_VALUE = (24000000U);
stduint HSI_VALUE = (64000000U);
#endif

namespace uni {
	using namespace RCCReg;
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
		return RCC[CR].bitof(_RCC_CR_POSI_HSEReady);
	}

	bool RCCOscillatorHSI::isReady() {
		return RCC[CR].bitof(_RCC_CR_POSI_HSIReady);
	}

	static byte _tab_AHB_PRES_EXPO[] = {
		0,  8,  9, 10, 11,  0, 12, 13, 14, 15
	};
	// divexpo is a great design of the past me! --dosconio 20240717
	bool RCCAHB::setMode(_TEMP uint8 divexpo, bool usingPCLK1, bool usingPCLK2) {
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

	RCCReg::RCCReg APBxDIVR[] = {
		APB1DIVR, APB2DIVR, APB3DIVR, APB4DIVR, APB5DIVR, APB6DIVR
	};
	
	void RCC_t::canMode() const {
		HSI.enAble(); while (!HSI.isReady());
		RCC[MCO1CFGR] = nil;
		RCC[MCO2CFGR] = nil;
		RCC[MPCKSELR].maset(0, 2, 0);// RCC_MPCKSELR_MPUSRC
		RCC[ASSCKSELR].maset(0, 3, 0);;// RCC_ASSCKSELR_AXISSRC
		//{} ifdef RCC_MSSCKSELR_MLAHBSSRC : MODIFY_REG(RCC->MSSCKSELR, (RCC_MSSCKSELR_MLAHBSSRC), 0U);
		RCC[MPCKDIVR].maset(0, 4, 1);// RCC_MPCKDIVR_MPUDIV -> RCC_MPCKDIVR_MPUDIV_0
		RCC[AXIDIVR].maset(0, 3, 0);// RCC_AXIDIVR_AXIDIV -> 0
		RCC[APB4DIVR].maset(0, 3, 0);// RCC_APB4DIVR_APB4DIV -> 0
		RCC[APB5DIVR].maset(0, 3, 0);// RCC_APB5DIVR_APB5DIV -> 0
		//{} ifdef RCC_MLAHBDIVR_MLAHBDIV : MODIFY_REG(RCC->MLAHBDIVR, (RCC_MLAHBDIVR_MLAHBDIV), 0U);
		RCC[APB1DIVR].maset(0, 3, 0);// RCC_APB1DIVR_APB1DIV -> 0
		RCC[APB2DIVR].maset(0, 3, 0);// RCC_APB2DIVR_APB2DIV -> 0
		RCC[APB3DIVR].maset(0, 3, 0);// RCC_APB3DIVR_APB3DIV -> 0
		//{} ifdef RCC_APB6DIVR_APB6DIV : MODIFY_REG(RCC->APB6DIVR, (RCC_APB6DIVR_APB6DIV), 0U);
		PLL1.canMode();
		PLL2.canMode();
		PLL3.canMode();
		PLL4.canMode();
		HSI.Reset();// canMode will set state OFF, not default state
		RCC[OCENCLRR] = _IMM(_OCENCLRR::_MASK) & (~_IMM1);// except HSION
		RCC[RDLSICR].setof(0, false);// LSION
		RCC[CSICFGR].maset(8, 5, 0);// CSI TRIM
		// ---- A7
		RCC[MP_CIER] = _MP_CIxR::_MASK;
		RCC[MP_CIFR] = _MP_CIxR::_MASK;
		RCC[MP_RSTSCLRR] = _IMM(_MP_MP_RSTSyR::_MASK) & 0x1FFF;// except SPARE
		// ---- A7 END
		SystemCoreClock = HSI_VALUE;
		if (!SysTick::enClock(SysTickHz));
	}

	// TEMP AREA ----

	bool AXISS_t::setMode(AxisSource source, byte divr) {
		bool state = true;
		if (!Ranglin(divr, 1, 4)) return false;
		switch (source) {
		case AxisSource::HSI:
			if (!RCC.HSI.isReady()) return false;
			break;
		case AxisSource::HSE:
			if (!RCC.HSE.isReady()) return false;
			break;
		case AxisSource::PLL2:
			if (!RCC.PLL2.isReady()) return false;
			break;
		default:// OFF
			state = false;
			break;
		}
		setSource(source);
		while (state != isReady());
		setDiv(divr);
		return true;
	}
	bool MLAHB_t::setMode(MLAHBSource source, byte divexpo) {
		if (divexpo > 0b111) return false;
		switch (source) {
		case MLAHBSource::HSI:
			if (!RCC.HSI.isReady()) return false;
			break;
		case MLAHBSource::HSE:
			if (!RCC.HSE.isReady()) return false;
			break;
		case MLAHBSource::CSI:
			if (!RCC.CSI.isReady()) return false;
			break;
		case MLAHBSource::PLL3:
			if (!RCC.PLL3.isReady()) return false;
			break;
		default:// OFF
			break;
		}
		setSource(source);
		while (!isReady());
		setDiv(divexpo);
		return true;
	}
	bool RCCAPB::setMode(byte divexpo) {
		if (divexpo > 4) return false;
		byte id = getID() - 1;
		// AKA AKA __HAL_RCC_APBx_DIV 
		RCC[APBxDIVR[id]].maset(0, 3, divexpo);// APBxDIV
		// AKA RCC_FLAG_APBxDIVRDY
		while (!RCC[APBxDIVR[id]].bitof(31));// APBxDIVRDY
		return true;
	}

	
	AxisSource AXISS_t::getSource() const {
		return (AxisSource)RCC[ASSCKSELR].masof(0, 3);// AXISSRC
	}
	MLAHBSource MLAHB_t::getSource() const {
		return (MLAHBSource)RCC[MSSCKSELR].masof(0, 2);// MLAHBSSRC
	}

	void AXISS_t::setSource(AxisSource source) const {
		RCC[ASSCKSELR].maset(0, 3, _IMM(source));// AXISSRC
	}
	void MLAHB_t::setSource(MLAHBSource source) const {
		RCC[MSSCKSELR].maset(0, 2, _IMM(source));// MLAHBS SRC
	}

	stduint AXISS_t::getFrequency() const {
		switch (getSource()) {
		case AxisSource::HSI:
			return RCC.HSI.getFrequency();
		case AxisSource::HSE:
			return RCC.HSE.getFrequency();
		case AxisSource::PLL2:
			return RCC.PLL2.getFrequencyP();
		case AxisSource::OFF: default: return 0;
		}
	}

	bool AXISS_t::isReady() const {
		return RCC[ASSCKSELR].bitof(31);// AXISSRCSRDY
	}
	bool MLAHB_t::isReady() const {
		return RCC[MSSCKSELR].bitof(31);// MLAHBSSRCSRDY
	}

	void AXISS_t::setDiv(byte divr) const {
		RCC[AXIDIVR].maset(0, 3, divr);//   AXIDIV
		while (!RCC[AXIDIVR].bitof(31)); // AXIDIVRD
	}
	void MLAHB_t::setDiv(byte divr) const {
		RCC[MLAHBDIVR].maset(0, 4, divr);//   MLAHB DIV
		while (!RCC[MLAHBDIVR].bitof(31)); // MLAHB DIV RD
	}

	void RCC_t::enSyscfg(bool ena) const {
		using namespace RCCReg;
		RCC[ena ? MP_NS_APB3ENSETR : MP_NS_APB3ENCLRR] = _IMM1S(0);
	}

#endif

	#include "../../../inc/c/driver/RCC/RCC-setClock.hpp"
		
	#if defined(_MCU_STM32)
		RCC_t RCC;
	#endif
}

