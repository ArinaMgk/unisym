// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Watching Dog: Independent and Window
// Codifiers: @ArinaMgk
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

#include "../../../inc/cpp/Device/Watchdog"
#include "../../../inc/cpp/Device/RCC/RCCAddress"
#include "../../../inc/cpp/Device/NVIC"
#if defined(_MCU_STM32H7x)
#include "../../../inc/cpp/Device/SysTick"
#endif

namespace uni {

#if defined(_MCU_STM32H7x)
	#define _IWDG1_ADDR 0x50004800
	#define _WWDG1_ADDR 0x58003000

	// IWDG KR key values
	#define _IWDG_KR_START   0xCCCC
	#define _IWDG_KR_UNLOCK  0x5555
	#define _IWDG_KR_RELOAD  0xAAAA

	// WWDG register bit positions (from RM0433)
	#define _WWDG_CR_WDGA_Pos   7
	#define _WWDG_CFR_EWI_Pos   9
	#define _WWDG_CFR_WDGTB_Pos 11
	// RCC APB3ENR WWDG1EN bit
	#define _RCC_APB3ENR_WWDG1EN_Pos 6U

	// ---- IWDG ---------------------------------------------------------------

	Reference IWDG_t::operator[](IWDGReg idx) const {
		return Reference(_IWDG1_ADDR + _IMMx4(idx));
	}

	// AKA HAL_IWDG_Init
	bool IWDG_t::setMode(IWDGPrescaler prescaler, stduint reload, stduint window) const {
		// 1. Start IWDG (forces LSI ON)
		self[IWDGReg::KR] = _IWDG_KR_START;
		// 2. Unlock write access to PR/RLR/WINR
		self[IWDGReg::KR] = _IWDG_KR_UNLOCK;
		// 3. Write Prescaler and Reload
		self[IWDGReg::PR] = (byte)prescaler;
		self[IWDGReg::RLR] = reload;
		// 4. Wait for registers to be updated (SR=0)
		uint64 tickstart = SysTick::getTick();
		while (self[IWDGReg::SR] != 0) {
			if ((SysTick::getTick() - tickstart) > _IWDG_TIMEOUT_VALUE) {
				return false;
			}
		}
		// 5. Window: write if changed, else reload to start with correct timebase
		if (self[IWDGReg::WINR] != window) {
			self[IWDGReg::WINR] = window;
		}
		else {
			self[IWDGReg::KR] = _IWDG_KR_RELOAD;
		}
		return true;
	}

	// AKA HAL_IWDG_Refresh
	bool IWDG_t::Refresh() const {
		self[IWDGReg::KR] = _IWDG_KR_RELOAD;
		return true;
	}

	bool IWDG_t::isReady() const {
		return self[IWDGReg::SR] == 0;
	}

	// ---- WWDG ---------------------------------------------------------------

	Reference WWDG_t::operator[](WWDGReg idx) const {
		return Reference(_WWDG1_ADDR + _IMMx4(idx));
	}

	bool WWDG_t::enClock(bool ena) const {
		Reference(_RCC_ADDR + _IMMx4(RCCReg::APB3ENR))
			.setof(_RCC_APB3ENR_WWDG1EN_Pos, ena);
		if (ena != Reference(_RCC_ADDR + _IMMx4(RCCReg::APB3ENR))
			.bitof(_RCC_APB3ENR_WWDG1EN_Pos))
			return false;
		return true;
	}

	// AKA HAL_WWDG_Init
	bool WWDG_t::setMode(WWDGPrescaler prescaler, stduint counter_val, stduint window) const {
		enClock();
		// Store counter for later Refresh
		((WWDG_t&)self).counter = (byte)counter_val;
		// CR: WDGA (bit7) | counter (bits 6:0)
		self[WWDGReg::CR] = (1U << _WWDG_CR_WDGA_Pos) | counter_val;
		// CFR: prescaler (bits 13:11) | window (bits 6:0), EWI off by default
		self[WWDGReg::CFR] = ((stduint)(byte)prescaler << _WWDG_CFR_WDGTB_Pos) | window;
		return true;
	}

	// AKA HAL_WWDG_Refresh
	bool WWDG_t::Refresh() const {
		self[WWDGReg::CR] = counter;
		return true;
	}

	bool WWDG_t::enInterrupt(bool ena) const {
		self[WWDGReg::CFR].setof(_WWDG_CFR_EWI_Pos, ena);
		return true;
	}

	void WWDG_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(IRQ_WWDG, preempt, sub_priority);
	}

	bool WWDG_t::enInterruptNVIC(bool ena) const {
		NVIC.setAble(IRQ_WWDG, ena);
		return true;
	}

	// AKA HAL_WWDG_IRQHandler
	void WWDG_t::IRQHandler() const {
		// Check EWI enable (CFR bit 9) and EWIF flag (SR bit 0)
		if (self[WWDGReg::CFR].bitof(_WWDG_CFR_EWI_Pos)
			&& self[WWDGReg::SR].bitof(0))
		{
			self[WWDGReg::SR] = 0; // clear EWIF (write 0)
			asserv(EarlyWakeupCallback)();
		}
	}

	IWDG_t IWDG1(1);
	WWDG_t WWDG1(1);

#endif
}

#if defined(_MCU_STM32H7x)
// vector table refers to this symbol; IRQ handler dispatches to WWDG1
void WWDG_IRQHandler(void) {
	uni::WWDG1.IRQHandler();
}
#endif
