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
#include "../../../../inc/cpp/Device/AFIO"
#include "../../../../inc/cpp/Device/Flash"
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/Device/SysTick"
#include "../../../../inc/cpp/Device/_Power.hpp"
#include "../../../../inc/c/driver/RCC/RCC-registers.hpp"

namespace uni {
#if 0
#elif defined(_MCU_STM32H7x)
#include "../../../../inc/cpp/Device/Power/Power-STM32H7.hpp"




	
	bool RCC_t::setClock(SysclkSource::RCCSysclockSource source) {
		// run this multiple times for different sources
		PWR_CR3_SCUEN = 0;
		PWR.ConfigVoltageScaling(1);
		while (!_IMM(PWR_D3CR_VOSRDY));
		// HAL_RCC_OscConfig switch
		{
			HSE.setMode();
			PLL1.setModePLL(PLLSource::HSE, 2, 0,
				.m = 5, .n = 160, .p = 2,
				.q = 4, .r = 4);
		}
		if (_TEMP 0) switch (source) {
		case SysclkSource::HSE:
			HSE.setMode();
			PLL1.setModePLL(PLLSource::HSE, 2, 0,
				.m = 5, .n = 160, .p = 2,
				.q = 4, .r = 4);
			break;
		case SysclkSource::HSI: HSI.setMode(true, _TEMP 0, _TEMP 0); break;
		case SysclkSource::CSI: CSI.setMode(true, _TEMP 0); break;
		case SysclkSource::PLL1: (void _TEMP)PLL1; break;
		// not for LSI, LSE, HSI48
		default: return false;
		}
		// HAL_RCC_ClockConfig block
		{
			uint32 _TEMP FLatency = _TEMP 2;// flash latency
			asrtret(FLatency < 8);// 0..8
			/* To correctly read data from FLASH memory, the number of wait states (LATENCY)
				  must be correctly programmed according to the frequency of the CPU clock
				  (HCLK) and the supply voltage of the device. */
			/* Increasing the CPU frequency */
			Stdfield FLASH_ACR_LATENCY(Flash[FlashReg::ACR], 0, 3);
			if (FLatency > _IMM(FLASH_ACR_LATENCY))
			{
				// Program the new number of wait states to the LATENCY bits in the FLASH_ACR register
				FLASH_ACR_LATENCY = FLatency;
				// Check that the new number of wait states is taken into account to access the Flash memory by reading the FLASH_ACR register
				asrtret(_IMM(FLASH_ACR_LATENCY) == FLatency)
			}
			AHB.setMode(1);
			Sysclock.setMode();
			/* Decreasing the number of wait states because of lower CPU frequency */
			if (FLatency < _IMM(FLASH_ACR_LATENCY))
			{
				FLASH_ACR_LATENCY = FLatency;
				asrtret(_IMM(FLASH_ACR_LATENCY) == FLatency)
			}
			D1PCLK1.setMode(1);// DIV2
			PCLK1.setMode(1);// DIV2
			PCLK2.setMode(1);// DIV2
			D3PCLK1.setMode(2);// APB4 DIV4
			Sysclock.getCoreFrequency();
		}
		CSI.enAble();
		RCC_APB4ENR_SYSCFGEN = 1; stduint tmp = RCC_APB4ENR_SYSCFGEN;// __HAL_RCC_SYSCFG_CLK_ENABLE();
		SYSCFG->CCCSR |= 0x00000001;//SYSCFG_CCCSR_EN, HAL_EnableCompensationCell();
		return SysTick::enClock(1000);// default 1kHz
	}

#endif
}
