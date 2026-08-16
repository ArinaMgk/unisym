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
#define _MCU_RCC_TEMP
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
stduint LSE_VALUE = (32768U);
stduint LSI_VALUE = (32000U);
stduint CSI_VALUE = (4000000U);
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
	using namespace RCCReg;
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
	using namespace RCCReg;

	bool RCCOscillatorHSE::isReady() {
		return RCC[CR].bitof(_RCC_CR_POSI_HSEReady);
	}

	bool RCCOscillatorHSI::isReady() {
		return RCC[CR].bitof(_RCC_CR_POSI_HSIReady);
	}

	static byte _tab_AHB_PRES_EXPO[] = {
		0,  8,  9, 10, 11,  0, 12, 13, 14, 15
	};
	// divexpo is a mac-friendly design of the past mine! --dosconio 20240717
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

#elif defined(_MCU_STM32H7x)

	bool RCCAHB::setMode(uint8 divexpo) {
		static int lst[]{
			0x0, 0x8, 0x9, 0xa, 0xb,
			-1,
			0xc, 0xd, 0xe, 0xf
		};
		asrtret(divexpo < numsof(lst) && lst[divexpo] >= 0);
		RCC_D1CFGR_HPRE = lst[divexpo];
		return true;
	}

	bool RCCAPB::setMode(uint8 divexpo) {
		asrtret(divexpo < 5);
		if (divexpo) divexpo += 3;
		switch (PCLK_ID) {
		case 0:
			RCC_D2CFGR_D2PPRE1 = divexpo; break;
		case 1:
			RCC_D2CFGR_D2PPRE2 = divexpo; break;
		case 2:// APB3
			RCC_D1CFGR_D1PPRE = divexpo; break;
		case 3:
			RCC_D3CFGR_D3PPRE = divexpo; break;
		default: return false;
		}


		return true;
	}

	
	stduint RCC_t::getFrequencyHCLK() {
		return SystemD2Clock = Sysclock.getCoreFrequency() >> D1CorePrescTable[_IMM(RCC_D1CFGR_HPRE)];
	}
	stduint RCC_t::getFrequencyPCLK1() {
		return (getFrequencyHCLK() >> D1CorePrescTable[_IMM(RCC_D2CFGR_D2PPRE1)]);
	}
	stduint RCC_t::getFrequencyPCLK2() {
		return (getFrequencyHCLK() >> D1CorePrescTable[_IMM(RCC_D2CFGR_D2PPRE2)]);
	}
	stduint RCC_t::getFrequencyD1PCLK1() {
		return (getFrequencyHCLK() >> D1CorePrescTable[_IMM(RCC_D1CFGR_D1PPRE)]);
	}
	stduint RCC_t::getFrequencyD3PCLK1() {
		return (getFrequencyHCLK() >> D1CorePrescTable[_IMM(RCC_D3CFGR_D3PPRE)]);
	}

	using namespace RCCReg;

	// Reset Mode (AKA HAL_RCC_DeInit)
	void RCC_t::canMode() const {
		RCC[CR].setof(0);// HSION
		RCC[CFGR] = nil;
		RCC[CR].rstof(1);// HSIKERON
		RCC[CR].maset(2, 2, 0);// HSIDIV
		RCC[CR].maset(4, 2, 0);// HSIDIVF
		RCC[CR].rstof(8);// CSION
		RCC[CR].rstof(9);// CSIKERON
		RCC[CR].rstof(12);// HSI48ON
		RCC[CR].rstof(16);// HSEON
		RCC[CR].rstof(18);// HSEBYP
		RCC[CR].rstof(19);// CSSHSEON
		RCC[CR].rstof(24);// PLL1ON
		RCC[CR].rstof(26);// PLL2ON
		RCC[CR].rstof(28);// PLL3ON
		RCC[D1CFGR] = nil;
		RCC[D2CFGR] = nil;
		RCC[D3CFGR] = nil;
		RCC[PLLCKSELR] = nil;
		RCC[PLLCFGR] = nil;
		RCC[PLL1DIVR] = nil;
		RCC[PLL1FRACR] = nil;
		RCC[PLL2DIVR] = nil;
		RCC[PLL2FRACR] = nil;
		RCC[PLL3DIVR] = nil;
		RCC[PLL3FRACR] = nil;
		RCC[CICR] = nil;
	}

	// PeriphCLK field descriptor (order MUST match PeriphClock)
	struct H7PeriphField { uni::RCCReg::RCCReg reg; byte pos; byte len; };
	static const H7PeriphField H7PeriphFields[] = {
		{ D2CCIP1R,  0, 3 },// SAI1
		{ D2CCIP1R,  6, 3 },// SAI23
		{ D2CCIP1R, 12, 3 },// SPI123
		{ D2CCIP1R, 16, 3 },// SPI45
		{ D3CCIPR,  28, 3 },// SPI6
		{ D2CCIP1R, 20, 2 },// SPDIFRX
		{ D2CCIP1R, 24, 1 },// DFSDM1
		{ D2CCIP1R, 28, 2 },// FDCAN
		{ D2CCIP1R, 31, 1 },// SWPMI1
		{ D2CCIP2R,  3, 3 },// USART16
		{ D2CCIP2R,  0, 3 },// USART234578
		{ D2CCIP2R,  8, 2 },// RNG
		{ D2CCIP2R, 12, 2 },// I2C123
		{ D3CCIPR,   8, 2 },// I2C4
		{ D2CCIP2R, 20, 2 },// USB
		{ D2CCIP2R, 22, 2 },// CEC
		{ D2CCIP2R, 28, 3 },// LPTIM1
		{ D3CCIPR,  10, 3 },// LPTIM2
		{ D3CCIPR,  13, 3 },// LPTIM345
		{ D3CCIPR,   0, 3 },// LPUART1
		{ D3CCIPR,  16, 2 },// ADC
		{ D3CCIPR,  21, 3 },// SAI4A
		{ D3CCIPR,  24, 3 },// SAI4B
		{ D1CCIPR,   0, 2 },// FMC
		{ D1CCIPR,   4, 2 },// QSPI
		{ D1CCIPR,  16, 1 },// SDMMC
		{ D1CCIPR,  28, 2 },// CKPER
	};

	// Map (clk, src) to raw field bits; 0xFF = invalid.
	static byte H7SourceBits(PeriphClock clk, ClockSource src) {
		switch (clk) {
		// SAI1/SAI23/SPI123/SAI4A/SAI4B: 0=PLL1, 1=PLL2, 2=PLL3, 3=Pin, 4=CKPER
		case PeriphClock::SAI1: case PeriphClock::SAI23: case PeriphClock::SPI123:
		case PeriphClock::SAI4A: case PeriphClock::SAI4B:
			switch (src) {
			case ClockSource::PLL1:  return 0;
			case ClockSource::PLL2:  return 1;
			case ClockSource::PLL3:  return 2;
			case ClockSource::Pin:   return 3;
			case ClockSource::CKPER: return 4;
			default: return 0xFF;
			}
		// SPI45/SPI6: 0=Bus, 1=PLL2, 2=PLL3, 3=HSI, 4=CSI, 5=HSE
		case PeriphClock::SPI45: case PeriphClock::SPI6:
			switch (src) {
			case ClockSource::Bus:  return 0;
			case ClockSource::PLL2: return 1;
			case ClockSource::PLL3: return 2;
			case ClockSource::HSI:  return 3;
			case ClockSource::CSI:  return 4;
			case ClockSource::HSE:  return 5;
			default: return 0xFF;
			}
		// USART16/USART234578/LPUART1: 0=Bus, 1=PLL2, 2=PLL3, 3=HSI, 4=CSI, 5=LSE
		case PeriphClock::USART16: case PeriphClock::USART234578: case PeriphClock::LPUART1:
			switch (src) {
			case ClockSource::Bus:  return 0;
			case ClockSource::PLL2: return 1;
			case ClockSource::PLL3: return 2;
			case ClockSource::HSI:  return 3;
			case ClockSource::CSI:  return 4;
			case ClockSource::LSE:  return 5;
			default: return 0xFF;
			}
		// LPTIM1/LPTIM2/LPTIM345: 0=Bus, 1=PLL2, 2=PLL3, 3=LSE, 4=LSI, 5=CKPER
		case PeriphClock::LPTIM1: case PeriphClock::LPTIM2: case PeriphClock::LPTIM345:
			switch (src) {
			case ClockSource::Bus:   return 0;
			case ClockSource::PLL2:  return 1;
			case ClockSource::PLL3:  return 2;
			case ClockSource::LSE:   return 3;
			case ClockSource::LSI:   return 4;
			case ClockSource::CKPER: return 5;
			default: return 0xFF;
			}
		// I2C123/I2C4: 0=Bus, 1=PLL3, 2=HSI, 3=CSI
		case PeriphClock::I2C123: case PeriphClock::I2C4:
			switch (src) {
			case ClockSource::Bus:  return 0;
			case ClockSource::PLL3: return 1;
			case ClockSource::HSI:  return 2;
			case ClockSource::CSI:  return 3;
			default: return 0xFF;
			}
		// SPDIFRX: 0=PLL1, 1=PLL2, 2=PLL3, 3=HSI
		case PeriphClock::SPDIFRX:
			switch (src) {
			case ClockSource::PLL1: return 0;
			case ClockSource::PLL2: return 1;
			case ClockSource::PLL3: return 2;
			case ClockSource::HSI:  return 3;
			default: return 0xFF;
			}
		// DFSDM1: 0=Bus, 1=Sys
		case PeriphClock::DFSDM1:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::Sys: return 1;
			default: return 0xFF;
			}
		// FDCAN: 0=HSE, 1=PLL1, 2=PLL2
		case PeriphClock::FDCAN:
			switch (src) {
			case ClockSource::HSE:  return 0;
			case ClockSource::PLL1: return 1;
			case ClockSource::PLL2: return 2;
			default: return 0xFF;
			}
		// SWPMI1: 0=Bus, 1=HSI
		case PeriphClock::SWPMI1:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::HSI: return 1;
			default: return 0xFF;
			}
		// RNG: 0=HSI48, 1=PLL1, 2=LSE, 3=LSI
		case PeriphClock::RNG:
			switch (src) {
			case ClockSource::HSI48: return 0;
			case ClockSource::PLL1:  return 1;
			case ClockSource::LSE:   return 2;
			case ClockSource::LSI:   return 3;
			default: return 0xFF;
			}
		// USB: 1=PLL1, 2=PLL3, 3=HSI48
		case PeriphClock::USB:
			switch (src) {
			case ClockSource::PLL1:  return 1;
			case ClockSource::PLL3:  return 2;
			case ClockSource::HSI48: return 3;
			default: return 0xFF;
			}
		// CEC: 0=LSE, 1=LSI, 2=CSI
		case PeriphClock::CEC:
			switch (src) {
			case ClockSource::LSE: return 0;
			case ClockSource::LSI: return 1;
			case ClockSource::CSI: return 2;
			default: return 0xFF;
			}
		// ADC: 0=PLL2, 1=PLL3, 2=CKPER
		case PeriphClock::ADC:
			switch (src) {
			case ClockSource::PLL2:  return 0;
			case ClockSource::PLL3:  return 1;
			case ClockSource::CKPER: return 2;
			default: return 0xFF;
			}
		// FMC/QSPI: 0=Bus, 1=PLL1, 2=PLL2, 3=CKPER
		case PeriphClock::FMC: case PeriphClock::QSPI:
			switch (src) {
			case ClockSource::Bus:   return 0;
			case ClockSource::PLL1:  return 1;
			case ClockSource::PLL2:  return 2;
			case ClockSource::CKPER: return 3;
			default: return 0xFF;
			}
		// SDMMC: 0=PLL1, 1=PLL2
		case PeriphClock::SDMMC:
			switch (src) {
			case ClockSource::PLL1: return 0;
			case ClockSource::PLL2: return 1;
			default: return 0xFF;
			}
		// CKPER: 0=HSI, 1=CSI, 2=HSE
		case PeriphClock::CKPER:
			switch (src) {
			case ClockSource::HSI: return 0;
			case ClockSource::CSI: return 1;
			case ClockSource::HSE: return 2;
			default: return 0xFF;
			}
		default: return 0xFF;
		}
	}

	// Map raw field bits back to ClockSource.
	static ClockSource H7BitsSource(PeriphClock clk, byte bits) {
		switch (clk) {
		case PeriphClock::SAI1: case PeriphClock::SAI23: case PeriphClock::SPI123:
		case PeriphClock::SAI4A: case PeriphClock::SAI4B:
			switch (bits) {
			case 1: return ClockSource::PLL2;
			case 2: return ClockSource::PLL3;
			case 3: return ClockSource::Pin;
			case 4: return ClockSource::CKPER;
			default: return ClockSource::PLL1;
			}
		case PeriphClock::SPI45: case PeriphClock::SPI6:
			switch (bits) {
			case 1: return ClockSource::PLL2;
			case 2: return ClockSource::PLL3;
			case 3: return ClockSource::HSI;
			case 4: return ClockSource::CSI;
			case 5: return ClockSource::HSE;
			default: return ClockSource::Bus;
			}
		case PeriphClock::USART16: case PeriphClock::USART234578: case PeriphClock::LPUART1:
			switch (bits) {
			case 1: return ClockSource::PLL2;
			case 2: return ClockSource::PLL3;
			case 3: return ClockSource::HSI;
			case 4: return ClockSource::CSI;
			case 5: return ClockSource::LSE;
			default: return ClockSource::Bus;
			}
		case PeriphClock::LPTIM1: case PeriphClock::LPTIM2: case PeriphClock::LPTIM345:
			switch (bits) {
			case 1: return ClockSource::PLL2;
			case 2: return ClockSource::PLL3;
			case 3: return ClockSource::LSE;
			case 4: return ClockSource::LSI;
			case 5: return ClockSource::CKPER;
			default: return ClockSource::Bus;
			}
		case PeriphClock::I2C123: case PeriphClock::I2C4:
			switch (bits) {
			case 1: return ClockSource::PLL3;
			case 2: return ClockSource::HSI;
			case 3: return ClockSource::CSI;
			default: return ClockSource::Bus;
			}
		case PeriphClock::SPDIFRX:
			switch (bits) {
			case 1: return ClockSource::PLL2;
			case 2: return ClockSource::PLL3;
			case 3: return ClockSource::HSI;
			default: return ClockSource::PLL1;
			}
		case PeriphClock::DFSDM1:
			return bits ? ClockSource::Sys : ClockSource::Bus;
		case PeriphClock::FDCAN:
			switch (bits) {
			case 1: return ClockSource::PLL1;
			case 2: return ClockSource::PLL2;
			default: return ClockSource::HSE;
			}
		case PeriphClock::SWPMI1:
			return bits ? ClockSource::HSI : ClockSource::Bus;
		case PeriphClock::RNG:
			switch (bits) {
			case 1: return ClockSource::PLL1;
			case 2: return ClockSource::LSE;
			case 3: return ClockSource::LSI;
			default: return ClockSource::HSI48;
			}
		case PeriphClock::USB:
			switch (bits) {
			case 2: return ClockSource::PLL3;
			case 3: return ClockSource::HSI48;
			default: return ClockSource::PLL1;
			}
		case PeriphClock::CEC:
			switch (bits) {
			case 1: return ClockSource::LSI;
			case 2: return ClockSource::CSI;
			default: return ClockSource::LSE;
			}
		case PeriphClock::ADC:
			switch (bits) {
			case 1: return ClockSource::PLL3;
			case 2: return ClockSource::CKPER;
			default: return ClockSource::PLL2;
			}
		case PeriphClock::FMC: case PeriphClock::QSPI:
			switch (bits) {
			case 1: return ClockSource::PLL1;
			case 2: return ClockSource::PLL2;
			case 3: return ClockSource::CKPER;
			default: return ClockSource::Bus;
			}
		case PeriphClock::SDMMC:
			return bits ? ClockSource::PLL2 : ClockSource::PLL1;
		case PeriphClock::CKPER:
			switch (bits) {
			case 1: return ClockSource::CSI;
			case 2: return ClockSource::HSE;
			default: return ClockSource::HSI;
			}
		default: return ClockSource::Bus;
		}
	}

	// AKA HAL_RCCEx_PeriphCLKConfig
	bool RCC_t::setPeriphClock(PeriphClock clk, ClockSource src) const {
		if (clk == PeriphClock::RTC) {
			byte bits;
			switch (src) {
			case ClockSource::LSE: bits = 1; break;
			case ClockSource::LSI: bits = 2; break;
			case ClockSource::HSE: bits = 3; break;
			default: return false;
			}
			RCC[BDCR].maset(8, 2, bits);// RTCSEL
			return true;
		}
		byte idx = (byte)_IMM(clk);
		if (idx >= numsof(H7PeriphFields)) return false;
		byte bits = H7SourceBits(clk, src);
		if (bits == 0xFF) return false;
		const H7PeriphField& f = H7PeriphFields[idx];
		RCC[f.reg].maset(f.pos, f.len, bits);
		return true;
	}

	// AKA HAL_RCCEx_GetPeriphCLKConfig
	ClockSource RCC_t::getPeriphClock(PeriphClock clk) const {
		if (clk == PeriphClock::RTC) {
			switch ((byte)RCC[BDCR].masof(8, 2)) {
			case 1: return ClockSource::LSE;
			case 2: return ClockSource::LSI;
			case 3: return ClockSource::HSE;
			default: return ClockSource::Bus;// NoClock
			}
		}
		byte idx = (byte)_IMM(clk);
		if (idx >= numsof(H7PeriphFields)) return ClockSource::Bus;
		const H7PeriphField& f = H7PeriphFields[idx];
		return H7BitsSource(clk, (byte)RCC[f.reg].masof(f.pos, f.len));
	}

	// AKA HAL_RCCEx_GetPeriphCLKFreq (SAI/SPI123 family only)
	stduint RCC_t::getPeriphClockFreq(PeriphClock clk) const {
		switch (clk) {
		case PeriphClock::SAI1: case PeriphClock::SAI23:
		case PeriphClock::SAI4A: case PeriphClock::SAI4B:
		case PeriphClock::SPI123: break;
		default: return 0;
		}
		const H7PeriphField& f = H7PeriphFields[_IMM(clk)];
		switch ((byte)RCC[f.reg].masof(f.pos, f.len)) {
		case 0: return PLL1.getFrequencyQ();
		case 1: return PLL2.getFrequencyP();
		case 2: return PLL3.getFrequencyP();
		case 3: return 12288000U;// Pin (external audio clock)
		case 4:
			switch ((byte)RCC[D1CCIPR].masof(28, 2)) {// CKPERSEL
			case 0: return HSI_VALUE;
			case 1: return CSI_VALUE;
			default: return HSE_VALUE;
			}
		default: return 0;
		}
	}

	// AKA HAL_RCCEx_WakeUpStopCLKConfig (0=HSI, non-zero=CSI)
	void RCC_t::setWakeUpClock(uint32 clk) const {
		RCC[CFGR].setof(6, clk != 0);// STOPWUCK
	}

	// AKA HAL_RCCEx_KerWakeUpStopCLKConfig (0=HSI, non-zero=CSI)
	void RCC_t::setKernelWakeUpClock(uint32 clk) const {
		RCC[CFGR].setof(7, clk != 0);// STOPKERWUCK
	}

	// AKA HAL_RCCEx_EnableLSECSS / DisableLSECSS
	void RCC_t::enLSECSS(bool ena) const {
		RCC[BDCR].setof(5, ena);// LSECSSON
		if (!ena) RCC[CIER].rstof(9);// LSECSSIE
	}

	// AKA HAL_RCC_EnableCSS
	void RCC_t::enHSECSS(bool ena) const {
		RCC[CR].setof(19, ena);// CSSHSEON
	}

	// AKA HAL_RCCEx_WWDGxSysResetConfig
	void RCC_t::setWWDGReset(uint32 wwdg) const {
		if (wwdg) RCC[GCR].setof(0);// WW1RSC
	}

	// AKA HAL_RCCEx_CRSWaitSynchronization (independent CRS peripheral)
	bool RCC_t::PollCRS(uint32 timeout) const {
		Reference isr(0x40004800 + 0x08);// CRS_ISR
		Reference icr(0x40004800 + 0x0C);// CRS_ICR
		uint64 start = SysTick::getTick();
		do {
			if (isr.bitof(0)) {// SYNCOKF
				icr.setof(0);
				return true;
			}
			if (isr.bitof(10)) {// TRIMOVF
				icr.setof(10);
				return false;
			}
			if (isr.bitof(8)) {// SYNCERR
				icr.setof(8);
				return false;
			}
			if (isr.bitof(9)) {// SYNCMISS
				icr.setof(9);
				return false;
			}
		} while (timeout && (SysTick::getTick() - start) <= timeout);
	return false;
	}

	// AKA HAL_RCC_MCOConfig
	void RCC_t::setMCO(MCO1Source::MCO1Source src, MCODiv::MCODiv div) const {
		RCC[CFGR].maset(22, 3, _IMM(src));// MCO1SEL
		RCC[CFGR].maset(18, 4, _IMM(div));// MCO1PRE
	}
	void RCC_t::setMCO(MCO2Source::MCO2Source src, MCODiv::MCODiv div) const {
		RCC[CFGR].maset(29, 3, _IMM(src));// MCO2SEL
		RCC[CFGR].maset(25, 4, _IMM(div));// MCO2PRE
	}

	// AKA HAL_RCCEx_CRSConfig (independent CRS peripheral)
	void RCC_t::setCRS(CRSDiv::CRSDiv prescaler, CRSSource::CRSSource source, CRSPolarity::CRSPolarity polarity, uint32 reload, byte errorLimit, byte hsi48Calib) const {
		// Reset CRS registers to default values
		RCC[APB1HRSTR].setof(1);// CRSRST
		RCC[APB1HRSTR].rstof(1);
		Reference cfgr(0x40004800 + 0x00);// CRS_CFGR
		Reference cr(0x40004800 + 0x04);// CRS_CR
		cfgr.maset(24, 3, _IMM(prescaler));// SYNCDIV
		cfgr.maset(28, 2, _IMM(source));// SYNCSRC
		cfgr.maset(31, 1, _IMM(polarity));// SYNCPOL
		cfgr.maset(0, 16, reload);// RELOAD
		cfgr.maset(16, 8, errorLimit);// FELIM
		cr.maset(8, 6, hsi48Calib);// TRIM
		cr.setof(6);// AUTOTRIMEN
		cr.setof(5);// CEN
	}

	// AKA HAL_RCCEx_CRSSoftwareSynchronizationGenerate
	void RCC_t::setCRSSync() const {
		Reference cr(0x40004800 + 0x04);// CRS_CR
		cr.setof(7);// SWSYNC
	}

	// AKA HAL_RCCEx_CRSGetSynchronizationInfo
	stduint RCC_t::getCRSReload() const {
		return Reference(0x40004800 + 0x00).mask(0, 16);// CRS_CFGR RELOAD
	}
	stduint RCC_t::getCRSCalibration() const {
		return Reference(0x40004800 + 0x04).masof(8, 6);// CRS_CR TRIM
	}
	stduint RCC_t::getCRSFreqError() const {
		return Reference(0x40004800 + 0x08).masof(16, 16);// CRS_ISR FECAP
	}
	bool RCC_t::getCRSFreqErrorDir() const {
		return Reference(0x40004800 + 0x08).bitof(15);// CRS_ISR FEDIR
	}

#elif defined(_MPU_STM32MP13)
	using namespace RCCReg;
	uni::RCCReg::RCCReg APBxDIVR[] = {
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
		if (!SysTick::enClock(SysTickHz)) return;
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

	stduint RCCAPB::getFrequency() const {
		byte id = getID();// 1~6
		// APB4/5 derive from ACLK; APB1/2/3/6 from MLAHB (post MLAHBDIV)
		stduint parent = (id == 4 || id == 5)
			? (RCC.AXIS.getFrequency() / (RCC[AXIDIVR].masof(0, 3) + 1))// ACLK
			: RCC.MLAHB.getFrequency();
		return parent >> RCC[APBxDIVR[id - 1]].masof(0, 3);// APBxDIV
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

	stduint MLAHB_t::getSourceFrequency() const {
		switch (getSource()) {
		case MLAHBSource::HSI: return RCC.HSI.getFrequency();
		case MLAHBSource::HSE: return RCC.HSE.getFrequency();
		case MLAHBSource::CSI: return CSI_VALUE;
		case MLAHBSource::PLL3: return RCC.PLL3.getFrequencyP();
		default: return 0;
		}
	}

	stduint MLAHB_t::getFrequency() const {
		return getSourceFrequency() >> RCC[MLAHBDIVR].masof(0, 4);// MLAHBDIV (post div)
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

	// AKA HAL_RCC_GetXxxFreq series
	stduint RCC_t::getFrequency(ClockDomain domain) const {
		stduint aclk = AXIS.getFrequency() / (RCC[AXIDIVR].masof(0, 3) + 1);// ACLK
		stduint mlahb = MLAHB.getFrequency();// MLAHB (post MLAHBDIV)
		switch (domain) {
		case ClockDomain::MPU:
			return Sysclock.getCoreFrequency();
		case ClockDomain::AXIS:
			return AXIS.getFrequency();
		case ClockDomain::MLAHB:
			return mlahb;
		case ClockDomain::ACLK:
			return aclk;
		case ClockDomain::FCLK:
		case ClockDomain::MLHCLK:
			return mlahb;
		case ClockDomain::HCLK1: case ClockDomain::HCLK2:
		case ClockDomain::HCLK3: case ClockDomain::HCLK4:
			return mlahb;
		case ClockDomain::HCLK5: case ClockDomain::HCLK6:
			return aclk;
		case ClockDomain::PCLK1: return mlahb >> RCC[APB1DIVR].masof(0, 3);
		case ClockDomain::PCLK2: return mlahb >> RCC[APB2DIVR].masof(0, 3);
		case ClockDomain::PCLK3: return mlahb >> RCC[APB3DIVR].masof(0, 3);
		case ClockDomain::PCLK4: return aclk >> RCC[APB4DIVR].masof(0, 3);
		case ClockDomain::PCLK5: return aclk >> RCC[APB5DIVR].masof(0, 3);
		case ClockDomain::PCLK6: return mlahb >> RCC[APB6DIVR].masof(0, 3);
		default: return 0;
		}
	}

	// PeriphCLK field descriptor (order MUST match PeriphClock).
	// Aggregated selectors (I2C345/SPI45/UART12/LPTIM23/SDMMC12/ADC12/ETH12)
	// operate on the first sub-peripheral field (e.g. I2C3, SPI4, USART1, ...).
	struct MP13PeriphField { uni::RCCReg::RCCReg reg; byte pos; byte len; };
	static const MP13PeriphField MP13PeriphFields[] = {
		{ I2C12CKSELR,   0, 3 },// I2C12
		{ I2C345CKSELR,  0, 3 },// I2C345 (I2C3 sub-field)
		{ SAI1CKSELR,    0, 3 },// SAI1
		{ SAI2CKSELR,    0, 3 },// SAI2
		{ SPI2S1CKSELR,  0, 3 },// SPI1
		{ SPI2S23CKSELR, 0, 3 },// SPI23
		{ SPI45CKSELR,   0, 3 },// SPI45 (SPI4 sub-field)
		{ UART12CKSELR,  0, 3 },// UART12 (USART1 sub-field)
		{ UART35CKSELR,  0, 3 },// UART35
		{ UART4CKSELR,   0, 3 },// UART4
		{ UART6CKSELR,   0, 3 },// UART6
		{ UART78CKSELR,  0, 3 },// UART78
		{ QSPICKSELR,    0, 2 },// QSPI
		{ FMCCKSELR,     0, 2 },// FMC
		{ FDCANCKSELR,   0, 2 },// FDCAN
		{ SPDIFCKSELR,   0, 2 },// SPDIFRX
		{ CPERCKSELR,    0, 2 },// CKPER
		{ STGENCKSELR,   0, 2 },// STGEN
		{ DCMIPPCKSELR,  0, 2 },// DCMIPP
		{ SAESCKSELR,    0, 2 },// SAES
		{ LPTIM1CKSELR,  0, 3 },// LPTIM1
		{ LPTIM23CKSELR, 0, 3 },// LPTIM23 (LPTIM2 sub-field)
		{ LPTIM45CKSELR, 0, 3 },// LPTIM45
		{ SDMMC12CKSELR, 0, 3 },// SDMMC12 (SDMMC1 sub-field)
		{ ADC12CKSELR,   0, 2 },// ADC12 (ADC1 sub-field)
		{ ETH12CKSELR,   0, 2 },// ETH12 (ETH1 sub-field)
		{ USBCKSELR,     0, 2 },// USBPHY
	};

	// Map (clk, src) to raw field bits; 0xFF = invalid.
	static byte MP13SourceBits(PeriphClock clk, ClockSource src) {
		switch (clk) {
		// I2C12/I2C345: 0=Bus(PCLK1/PCLK6), 1=PLL4_R, 2=HSI, 3=CSI
		case PeriphClock::I2C12: case PeriphClock::I2C345:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::PLL4: case ClockSource::PLL4_R: return 1;
			case ClockSource::HSI: return 2;
			case ClockSource::CSI: return 3;
			default: return 0xFF;
			}
		// SAI1: 0=PLL4_Q, 1=PLL3_Q, 2=I2SCKIN, 3=PER, 4=PLL3_R
		case PeriphClock::SAI1:
			switch (src) {
			case ClockSource::PLL4: case ClockSource::PLL4_Q: return 0;
			case ClockSource::PLL3: case ClockSource::PLL3_Q: return 1;
			case ClockSource::I2SCKIN: return 2;
			case ClockSource::PER: return 3;
			case ClockSource::PLL3_R: return 4;
			default: return 0xFF;
			}
		// SAI2: 0=PLL4_Q, 1=PLL3_Q, 2=I2SCKIN, 3=PER, 4=SPDIF, 5=PLL3_R
		case PeriphClock::SAI2:
			switch (src) {
			case ClockSource::PLL4: case ClockSource::PLL4_Q: return 0;
			case ClockSource::PLL3: case ClockSource::PLL3_Q: return 1;
			case ClockSource::I2SCKIN: return 2;
			case ClockSource::PER: return 3;
			case ClockSource::SPDIF: return 4;
			case ClockSource::PLL3_R: return 5;
			default: return 0xFF;
			}
		// SPI1/SPI23: 0=PLL4_P, 1=PLL3_Q, 2=I2SCKIN, 3=PER, 4=PLL3_R
		case PeriphClock::SPI1: case PeriphClock::SPI23:
			switch (src) {
			case ClockSource::PLL4: case ClockSource::PLL4_P: return 0;
			case ClockSource::PLL3: case ClockSource::PLL3_Q: return 1;
			case ClockSource::I2SCKIN: return 2;
			case ClockSource::PER: return 3;
			case ClockSource::PLL3_R: return 4;
			default: return 0xFF;
			}
		// SPI45 (SPI4): 0=Bus(PCLK6), 1=PLL4_Q, 2=HSI, 3=CSI, 4=HSE, 5=I2SCKIN
		case PeriphClock::SPI45:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::PLL4: case ClockSource::PLL4_Q: return 1;
			case ClockSource::HSI: return 2;
			case ClockSource::CSI: return 3;
			case ClockSource::HSE: return 4;
			case ClockSource::I2SCKIN: return 5;
			default: return 0xFF;
			}
		// UART12 (USART1): 0=Bus(PCLK6), 1=PLL3_Q, 2=HSI, 3=CSI, 4=PLL4_Q, 5=HSE
		case PeriphClock::UART12:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::PLL3: case ClockSource::PLL3_Q: return 1;
			case ClockSource::HSI: return 2;
			case ClockSource::CSI: return 3;
			case ClockSource::PLL4: case ClockSource::PLL4_Q: return 4;
			case ClockSource::HSE: return 5;
			default: return 0xFF;
			}
		// UART35/UART4/UART6/UART78: 0=Bus, 1=PLL4_Q, 2=HSI, 3=CSI, 4=HSE
		case PeriphClock::UART35: case PeriphClock::UART4:
		case PeriphClock::UART6: case PeriphClock::UART78:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::PLL4: case ClockSource::PLL4_Q: return 1;
			case ClockSource::HSI: return 2;
			case ClockSource::CSI: return 3;
			case ClockSource::HSE: return 4;
			default: return 0xFF;
			}
		// QSPI/FMC: 0=Bus(ACLK), 1=PLL3_R, 2=PLL4_P, 3=PER
		case PeriphClock::QSPI: case PeriphClock::FMC:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::PLL3: case ClockSource::PLL3_R: return 1;
			case ClockSource::PLL4: case ClockSource::PLL4_P: return 2;
			case ClockSource::PER: return 3;
			default: return 0xFF;
			}
		// FDCAN: 0=HSE, 1=PLL3_Q, 2=PLL4_Q, 3=PLL4_R
		case PeriphClock::FDCAN:
			switch (src) {
			case ClockSource::HSE: return 0;
			case ClockSource::PLL3: case ClockSource::PLL3_Q: return 1;
			case ClockSource::PLL4_Q: return 2;
			case ClockSource::PLL4_R: return 3;
			default: return 0xFF;
			}
		// SPDIFRX: 0=PLL4_P, 1=PLL3_Q, 2=HSI
		case PeriphClock::SPDIFRX:
			switch (src) {
			case ClockSource::PLL4: case ClockSource::PLL4_P: return 0;
			case ClockSource::PLL3: case ClockSource::PLL3_Q: return 1;
			case ClockSource::HSI: return 2;
			default: return 0xFF;
			}
		// CKPER: 0=HSI, 1=CSI, 2=HSE, 3=OFF
		case PeriphClock::CKPER:
			switch (src) {
			case ClockSource::HSI: return 0;
			case ClockSource::CSI: return 1;
			case ClockSource::HSE: return 2;
			case ClockSource::OFF: return 3;
			default: return 0xFF;
			}
		// STGEN: 0=HSI, 1=HSE, 2=OFF
		case PeriphClock::STGEN:
			switch (src) {
			case ClockSource::HSI: return 0;
			case ClockSource::HSE: return 1;
			case ClockSource::OFF: return 2;
			default: return 0xFF;
			}
		// DCMIPP: 0=Bus(ACLK), 1=PLL2_Q, 2=PLL4_P, 3=PER
		case PeriphClock::DCMIPP:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::PLL2: return 1;
			case ClockSource::PLL4: case ClockSource::PLL4_P: return 2;
			case ClockSource::PER: return 3;
			default: return 0xFF;
			}
		// SAES: 0=Bus(ACLK), 1=PER, 2=PLL4_R, 3=LSI
		case PeriphClock::SAES:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::PER: return 1;
			case ClockSource::PLL4: case ClockSource::PLL4_R: return 2;
			case ClockSource::LSI: return 3;
			default: return 0xFF;
			}
		// LPTIM1: 0=Bus(PCLK1), 1=PLL4_P, 2=PLL3_Q, 3=LSE, 4=LSI, 5=PER, 6=OFF
		case PeriphClock::LPTIM1:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::PLL4: case ClockSource::PLL4_P: return 1;
			case ClockSource::PLL3: case ClockSource::PLL3_Q: return 2;
			case ClockSource::LSE: return 3;
			case ClockSource::LSI: return 4;
			case ClockSource::PER: return 5;
			case ClockSource::OFF: return 6;
			default: return 0xFF;
			}
		// LPTIM23 (LPTIM2): 0=Bus(PCLK3), 1=PLL4_Q, 2=PER, 3=LSE, 4=LSI, 5=OFF
		case PeriphClock::LPTIM23:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::PLL4: case ClockSource::PLL4_Q: return 1;
			case ClockSource::PER: return 2;
			case ClockSource::LSE: return 3;
			case ClockSource::LSI: return 4;
			case ClockSource::OFF: return 5;
			default: return 0xFF;
			}
		// LPTIM45: 0=Bus(PCLK3), 1=PLL4_P, 2=PLL3_Q, 3=LSE, 4=LSI, 5=PER, 6=OFF
		case PeriphClock::LPTIM45:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::PLL4: case ClockSource::PLL4_P: return 1;
			case ClockSource::PLL3: case ClockSource::PLL3_Q: return 2;
			case ClockSource::LSE: return 3;
			case ClockSource::LSI: return 4;
			case ClockSource::PER: return 5;
			case ClockSource::OFF: return 6;
			default: return 0xFF;
			}
		// SDMMC12 (SDMMC1): 0=Bus(HCLK6), 1=PLL3_R, 2=PLL4_P, 3=HSI
		case PeriphClock::SDMMC12:
			switch (src) {
			case ClockSource::Bus: return 0;
			case ClockSource::PLL3: case ClockSource::PLL3_R: return 1;
			case ClockSource::PLL4: case ClockSource::PLL4_P: return 2;
			case ClockSource::HSI: return 3;
			default: return 0xFF;
			}
		// ADC12 (ADC1): 0=PLL4_R, 1=PER, 2=PLL3_Q
		case PeriphClock::ADC12:
			switch (src) {
			case ClockSource::PLL4: case ClockSource::PLL4_R: return 0;
			case ClockSource::PER: return 1;
			case ClockSource::PLL3: case ClockSource::PLL3_Q: return 2;
			default: return 0xFF;
			}
		// ETH12 (ETH1): 0=PLL4_P, 1=PLL3_Q, 2=OFF
		case PeriphClock::ETH12:
			switch (src) {
			case ClockSource::PLL4: case ClockSource::PLL4_P: return 0;
			case ClockSource::PLL3: case ClockSource::PLL3_Q: return 1;
			case ClockSource::OFF: return 2;
			default: return 0xFF;
			}
		// USBPHY: 0=HSE, 1=PLL4_R (2=HSE/2 has no ClockSource member)
		case PeriphClock::USBPHY:
			switch (src) {
			case ClockSource::HSE: return 0;
			case ClockSource::PLL4: case ClockSource::PLL4_R: return 1;
			default: return 0xFF;
			}
		default: return 0xFF;
		}
	}

	// Map raw field bits back to ClockSource.
	static ClockSource MP13BitsSource(PeriphClock clk, byte bits) {
		switch (clk) {
		case PeriphClock::I2C12: case PeriphClock::I2C345:
			switch (bits) {
			case 1: return ClockSource::PLL4_R;
			case 2: return ClockSource::HSI;
			case 3: return ClockSource::CSI;
			default: return ClockSource::Bus;
			}
		case PeriphClock::SAI1:
			switch (bits) {
			case 1: return ClockSource::PLL3_Q;
			case 2: return ClockSource::I2SCKIN;
			case 3: return ClockSource::PER;
			case 4: return ClockSource::PLL3_R;
			default: return ClockSource::PLL4_Q;
			}
		case PeriphClock::SAI2:
			switch (bits) {
			case 1: return ClockSource::PLL3_Q;
			case 2: return ClockSource::I2SCKIN;
			case 3: return ClockSource::PER;
			case 4: return ClockSource::SPDIF;
			case 5: return ClockSource::PLL3_R;
			default: return ClockSource::PLL4_Q;
			}
		case PeriphClock::SPI1: case PeriphClock::SPI23:
			switch (bits) {
			case 1: return ClockSource::PLL3_Q;
			case 2: return ClockSource::I2SCKIN;
			case 3: return ClockSource::PER;
			case 4: return ClockSource::PLL3_R;
			default: return ClockSource::PLL4_P;
			}
		case PeriphClock::SPI45:
			switch (bits) {
			case 1: return ClockSource::PLL4_Q;
			case 2: return ClockSource::HSI;
			case 3: return ClockSource::CSI;
			case 4: return ClockSource::HSE;
			case 5: return ClockSource::I2SCKIN;
			default: return ClockSource::Bus;
			}
		case PeriphClock::UART12:
			switch (bits) {
			case 1: return ClockSource::PLL3_Q;
			case 2: return ClockSource::HSI;
			case 3: return ClockSource::CSI;
			case 4: return ClockSource::PLL4_Q;
			case 5: return ClockSource::HSE;
			default: return ClockSource::Bus;
			}
		case PeriphClock::UART35: case PeriphClock::UART4:
		case PeriphClock::UART6: case PeriphClock::UART78:
			switch (bits) {
			case 1: return ClockSource::PLL4_Q;
			case 2: return ClockSource::HSI;
			case 3: return ClockSource::CSI;
			case 4: return ClockSource::HSE;
			default: return ClockSource::Bus;
			}
		case PeriphClock::QSPI: case PeriphClock::FMC:
			switch (bits) {
			case 1: return ClockSource::PLL3_R;
			case 2: return ClockSource::PLL4_P;
			case 3: return ClockSource::PER;
			default: return ClockSource::Bus;
			}
		case PeriphClock::FDCAN:
			switch (bits) {
			case 1: return ClockSource::PLL3_Q;
			case 2: return ClockSource::PLL4_Q;
			case 3: return ClockSource::PLL4_R;
			default: return ClockSource::HSE;
			}
		case PeriphClock::SPDIFRX:
			switch (bits) {
			case 1: return ClockSource::PLL3_Q;
			case 2: return ClockSource::HSI;
			default: return ClockSource::PLL4_P;
			}
		case PeriphClock::CKPER:
			switch (bits) {
			case 1: return ClockSource::CSI;
			case 2: return ClockSource::HSE;
			case 3: return ClockSource::OFF;
			default: return ClockSource::HSI;
			}
		case PeriphClock::STGEN:
			switch (bits) {
			case 1: return ClockSource::HSE;
			case 2: return ClockSource::OFF;
			default: return ClockSource::HSI;
			}
		case PeriphClock::DCMIPP:
			switch (bits) {
			case 1: return ClockSource::PLL2;
			case 2: return ClockSource::PLL4_P;
			case 3: return ClockSource::PER;
			default: return ClockSource::Bus;
			}
		case PeriphClock::SAES:
			switch (bits) {
			case 1: return ClockSource::PER;
			case 2: return ClockSource::PLL4_R;
			case 3: return ClockSource::LSI;
			default: return ClockSource::Bus;
			}
		case PeriphClock::LPTIM1:
			switch (bits) {
			case 1: return ClockSource::PLL4_P;
			case 2: return ClockSource::PLL3_Q;
			case 3: return ClockSource::LSE;
			case 4: return ClockSource::LSI;
			case 5: return ClockSource::PER;
			case 6: return ClockSource::OFF;
			default: return ClockSource::Bus;
			}
		case PeriphClock::LPTIM23:
			switch (bits) {
			case 1: return ClockSource::PLL4_Q;
			case 2: return ClockSource::PER;
			case 3: return ClockSource::LSE;
			case 4: return ClockSource::LSI;
			case 5: return ClockSource::OFF;
			default: return ClockSource::Bus;
			}
		case PeriphClock::LPTIM45:
			switch (bits) {
			case 1: return ClockSource::PLL4_P;
			case 2: return ClockSource::PLL3_Q;
			case 3: return ClockSource::LSE;
			case 4: return ClockSource::LSI;
			case 5: return ClockSource::PER;
			case 6: return ClockSource::OFF;
			default: return ClockSource::Bus;
			}
		case PeriphClock::SDMMC12:
			switch (bits) {
			case 1: return ClockSource::PLL3_R;
			case 2: return ClockSource::PLL4_P;
			case 3: return ClockSource::HSI;
			default: return ClockSource::Bus;
			}
		case PeriphClock::ADC12:
			switch (bits) {
			case 1: return ClockSource::PER;
			case 2: return ClockSource::PLL3_Q;
			default: return ClockSource::PLL4_R;
			}
		case PeriphClock::ETH12:
			switch (bits) {
			case 1: return ClockSource::PLL3_Q;
			case 2: return ClockSource::OFF;
			default: return ClockSource::PLL4_P;
			}
		case PeriphClock::USBPHY:
			switch (bits) {
			case 1: return ClockSource::PLL4_R;
			default: return ClockSource::HSE;// bit 2 (HSE/2) maps to HSE by approximation
			}
		default: return ClockSource::Bus;
		}
	}

	// AKA HAL_RCCEx_PeriphCLKConfig
	bool RCC_t::setPeriphClock(PeriphClock clk, ClockSource src) const {
		byte idx = (byte)_IMM(clk);
		if (idx >= numsof(MP13PeriphFields)) return false;
		byte bits = MP13SourceBits(clk, src);
		if (bits == 0xFF) return false;
		const MP13PeriphField& f = MP13PeriphFields[idx];
		RCC[f.reg].maset(f.pos, f.len, bits);
		return true;
	}

	// AKA HAL_RCCEx_GetPeriphCLKConfig
	ClockSource RCC_t::getPeriphClock(PeriphClock clk) const {
		byte idx = (byte)_IMM(clk);
		if (idx >= numsof(MP13PeriphFields)) return ClockSource::Bus;
		const MP13PeriphField& f = MP13PeriphFields[idx];
		return MP13BitsSource(clk, (byte)RCC[f.reg].masof(f.pos, f.len));
	}

	// AKA HAL_RCCEx_GetPeriphCLKFreq
	stduint RCC_t::getPeriphClockFreq(PeriphClock clk) const {
		byte idx = (byte)_IMM(clk);
		if (idx >= numsof(MP13PeriphFields)) return 0;
		const MP13PeriphField& f = MP13PeriphFields[idx];
		byte bits = (byte)RCC[f.reg].masof(f.pos, f.len);

		stduint aclk = AXIS.getFrequency() / (RCC[AXIDIVR].masof(0, 3) + 1);
		stduint mlahb = MLAHB.getFrequency();
		stduint pclk1 = mlahb >> RCC[APB1DIVR].masof(0, 3);
		stduint pclk2 = mlahb >> RCC[APB2DIVR].masof(0, 3);
		stduint pclk3 = mlahb >> RCC[APB3DIVR].masof(0, 3);
		stduint pclk6 = mlahb >> RCC[APB6DIVR].masof(0, 3);
		stduint hsi = HSI.getFrequency();
		stduint pll2q = PLL2.getFrequencyQ();
		stduint pll3q = PLL3.getFrequencyQ();
		stduint pll3r = PLL3.getFrequencyR();
		stduint pll4p = PLL4.getFrequencyP();
		stduint pll4q = PLL4.getFrequencyQ();
		stduint pll4r = PLL4.getFrequencyR();
		stduint ckper = 0;
		switch ((byte)RCC[CPERCKSELR].masof(0, 2)) {// CKPERSRC
		case 0: ckper = HSI_VALUE; break;
		case 1: ckper = CSI_VALUE; break;
		case 2: ckper = HSE_VALUE; break;
		default: ckper = 0; break;
		}

		switch (clk) {
		case PeriphClock::I2C12:
			switch (bits) { case 1: return pll4r; case 2: return hsi; case 3: return CSI_VALUE; default: return pclk1; }
		case PeriphClock::I2C345:
			switch (bits) { case 1: return pll4r; case 2: return hsi; case 3: return CSI_VALUE; default: return pclk6; }
		case PeriphClock::SAI1:
			switch (bits) { case 1: return pll3q; case 2: return 12288000U; case 3: return ckper; case 4: return pll3r; default: return pll4q; }
		case PeriphClock::SAI2:
			switch (bits) { case 1: return pll3q; case 2: return 12288000U; case 3: return ckper; case 4: return 0; case 5: return pll3r; default: return pll4q; }
		case PeriphClock::SPI1: case PeriphClock::SPI23:
			switch (bits) { case 1: return pll3q; case 2: return 12288000U; case 3: return ckper; case 4: return pll3r; default: return pll4p; }
		case PeriphClock::SPI45:
			switch (bits) { case 1: return pll4q; case 2: return hsi; case 3: return CSI_VALUE; case 4: return HSE_VALUE; case 5: return 12288000U; default: return pclk6; }
		case PeriphClock::UART12:
			switch (bits) { case 1: return pll3q; case 2: return hsi; case 3: return CSI_VALUE; case 4: return pll4q; case 5: return HSE_VALUE; default: return pclk6; }
		case PeriphClock::UART35: case PeriphClock::UART4: case PeriphClock::UART78:
			switch (bits) { case 1: return pll4q; case 2: return hsi; case 3: return CSI_VALUE; case 4: return HSE_VALUE; default: return pclk1; }
		case PeriphClock::UART6:
			switch (bits) { case 1: return pll4q; case 2: return hsi; case 3: return CSI_VALUE; case 4: return HSE_VALUE; default: return pclk2; }
		case PeriphClock::QSPI: case PeriphClock::FMC:
			switch (bits) { case 1: return pll3r; case 2: return pll4p; case 3: return ckper; default: return aclk; }
		case PeriphClock::FDCAN:
			switch (bits) { case 1: return pll3q; case 2: return pll4q; case 3: return pll4r; default: return HSE_VALUE; }
		case PeriphClock::SPDIFRX:
			switch (bits) { case 1: return pll3q; case 2: return hsi; default: return pll4p; }
		case PeriphClock::CKPER:
			return ckper;
		case PeriphClock::STGEN:
			switch (bits) { case 1: return HSE_VALUE; case 2: return 0; default: return hsi; }
		case PeriphClock::DCMIPP:
			switch (bits) { case 1: return pll2q; case 2: return pll4p; case 3: return ckper; default: return aclk; }
		case PeriphClock::SAES:
			switch (bits) { case 1: return ckper; case 2: return pll4r; case 3: return LSI_VALUE; default: return aclk; }
		case PeriphClock::LPTIM1:
			switch (bits) { case 1: return pll4p; case 2: return pll3q; case 3: return LSE_VALUE; case 4: return LSI_VALUE; case 5: return ckper; case 6: return 0; default: return pclk1; }
		case PeriphClock::LPTIM23:
			switch (bits) { case 1: return pll4q; case 2: return ckper; case 3: return LSE_VALUE; case 4: return LSI_VALUE; case 5: return 0; default: return pclk3; }
		case PeriphClock::LPTIM45:
			switch (bits) { case 1: return pll4p; case 2: return pll3q; case 3: return LSE_VALUE; case 4: return LSI_VALUE; case 5: return ckper; case 6: return 0; default: return pclk3; }
		case PeriphClock::SDMMC12:
			switch (bits) { case 1: return pll3r; case 2: return pll4p; case 3: return hsi; default: return aclk; }
		case PeriphClock::ADC12:
			switch (bits) { case 1: return ckper; case 2: return pll3q; default: return pll4r; }
		case PeriphClock::ETH12:
			switch (bits) { case 1: return pll3q; case 2: return 0; default: return pll4p; }
		case PeriphClock::USBPHY:
			switch (bits) { case 1: return pll4r; case 2: return HSE_VALUE / 2U; default: return HSE_VALUE; }
		default: return 0;
		}
	}

	// AKA HAL_RCCEx_EnableLSECSS / DisableLSECSS
	void RCC_t::enLSECSS(bool ena) const {
		RCC[BDCR].setof(_IMM(_BDCR::LSECSSON), ena);
	}

	// AKA HAL_RCC_EnableHSECSS
	void RCC_t::enHSECSS(bool ena) const {
		RCC[OCENSETR].setof(11, ena);// HSECSSON
	}

	// AKA HAL_RCC_MCOConfig
	void RCC_t::setMCO(MCO1Source::MCO1Source src, MCODiv::MCODiv div) const {
		RCC[MCO1CFGR].maset(0, 3, _IMM(src));// MCO1SEL
		RCC[MCO1CFGR].maset(4, 4, _IMM(div));// MCO1DIV
		RCC[MCO1CFGR].setof(12);// MCO1ON
	}
	void RCC_t::setMCO(MCO2Source::MCO2Source src, MCODiv::MCODiv div) const {
		RCC[MCO2CFGR].maset(0, 3, _IMM(src));// MCO2SEL
		RCC[MCO2CFGR].maset(4, 4, _IMM(div));// MCO2DIV
		RCC[MCO2CFGR].setof(12);// MCO2ON
	}

#endif

#include "../../../inc/c/driver/RCC/RCC-setClock.hpp"// Some use this
		
#if defined(_MCU_STM32)
	RCC_t RCC;
#endif
}

