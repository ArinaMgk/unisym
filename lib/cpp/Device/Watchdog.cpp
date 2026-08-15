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

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

#include "../../../inc/cpp/Device/RCC/RCCAddress"
#if defined(_MCU_STM32H7x)
#include "../../../inc/cpp/Device/NVIC"
#endif
#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
#include "../../../inc/cpp/Device/SysTick"
#endif
#if defined(_MPU_STM32MP13)
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../inc/cpp/interrupt"
#endif

namespace uni {

#if defined(_MCU_STM32H7x)
	#define _IWDG1_ADDR 0x58004800
	#define _WWDG1_ADDR 0x50003000
#elif defined(_MPU_STM32MP13)
	#define _IWDG1_ADDR (APB5_PERIPH_BASE + 0x3000)
	#define _IWDG2_ADDR (APB4_PERIPH_BASE + 0x2000)
#endif

	// IWDG KR key values
	#define _IWDG_KR_START   0xCCCC
	#define _IWDG_KR_UNLOCK  0x5555
	#define _IWDG_KR_RELOAD  0xAAAA

#if defined(_MPU_STM32MP13)
	// IWDG SR/EWCR bit positions (from RM0475)
	#define _IWDG_SR_EWIF_Pos   14
	#define _IWDG_EWCR_EWIC_Pos 14
	#define _IWDG_EWCR_EWIE_Pos 15
	// SR update flags: PVU(0) RVU(1) WVU(2) EWU(3); ONF(8) stays set once enabled
	#define _IWDG_SR_UPDATE_MASK 0x0Fu
#endif

#if defined(_MCU_STM32H7x)
	// WWDG register bit positions (from RM0433)
	#define _WWDG_CR_WDGA_Pos   7
	#define _WWDG_CFR_EWI_Pos   9
	#define _WWDG_CFR_WDGTB_Pos 11
	// RCC APB3ENR WWDG1EN bit
	#define _RCC_APB3ENR_WWDG1EN_Pos 6U
#endif

	// ---- IWDG ---------------------------------------------------------------

	Reference IWDG_t::operator[](IWDGReg idx) const {
#if defined(_MCU_STM32H7x)
		return Reference(_IWDG1_ADDR + _IMMx4(idx));
#else
		static const stduint _IWDG_BASE[] = { 0, _IWDG1_ADDR, _IWDG2_ADDR };
		return Reference(_IWDG_BASE[getID()] + _IMMx4(idx));
#endif
	}

	// AKA HAL_IWDG_Init
	bool IWDG_t::setMode(IWDGPrescaler prescaler, stduint reload, stduint window
#if defined(_MPU_STM32MP13)
		, stduint ewi
#endif
	) const {
		// 1. Start IWDG (forces LSI ON)
		self[IWDGReg::KR] = _IWDG_KR_START;
		// 2. Unlock write access to PR/RLR/WINR/EWCR
		self[IWDGReg::KR] = _IWDG_KR_UNLOCK;
		// 3. Write Prescaler and Reload
		self[IWDGReg::PR] = (byte)prescaler;
		self[IWDGReg::RLR] = reload;
#if defined(_MPU_STM32MP13)
		// 4. Configure Early Wakeup Interrupt (matches HAL_IWDG_Init):
		//    ewi == 0 -> disable (acknowledge pending EWIF via EWIC)
		//    ewi != 0 -> enable EWIE + acknowledge + set EWIT comparator value
		if (ewi == 0)
			self[IWDGReg::EWCR] = (1u << _IWDG_EWCR_EWIC_Pos);
		else
			self[IWDGReg::EWCR] = (1u << _IWDG_EWCR_EWIE_Pos) | (1u << _IWDG_EWCR_EWIC_Pos) | ewi;
#endif
		// 5. Wait for registers to be updated (PVU/RVU/WVU/EWU cleared)
		uint64 tickstart = SysTick::getTick();
#if defined(_MCU_STM32H7x)
		while (self[IWDGReg::SR] != 0) {
#else
		// ONF (bit8) stays set once enabled; wait only for PVU/RVU/WVU/EWU
		while (self[IWDGReg::SR] & _IWDG_SR_UPDATE_MASK) {
#endif
			if ((SysTick::getTick() - tickstart) > _IWDG_TIMEOUT_VALUE) {
				return false;
			}
		}
		// 6. Window: write if changed, else reload to start with correct timebase
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
#if defined(_MCU_STM32H7x)
		return self[IWDGReg::SR] == 0;
#else
		// PVU/RVU/WVU/EWU cleared; ONF (bit8) remains set after enable
		return (self[IWDGReg::SR] & _IWDG_SR_UPDATE_MASK) == 0;
#endif
	}

#if defined(_MPU_STM32MP13)
	static Request_t _IWDG_Request_list[] = { Request_None, IRQ_IWDG1, IRQ_IWDG2 };

	// EWCR.EWIE set/clear (the only interrupt source); EWIT threshold set by setMode(..., ewi)
	bool IWDG_t::enInterrupt(bool ena) const {
		self[IWDGReg::KR] = _IWDG_KR_UNLOCK;
		self[IWDGReg::EWCR].setof(_IWDG_EWCR_EWIE_Pos, ena);
		return true;
	}

	void IWDG_t::setInterruptPriority(byte preempt, byte sub_priority) const {
		(void)sub_priority;
		GIC.setPriority(_IWDG_Request_list[getID()], preempt);
	}

	bool IWDG_t::enInterruptGIC(bool ena) const {
		GIC.enInterrupt(_IWDG_Request_list[getID()], ena);
		return true;
	}

	// AKA HAL_IWDG_IRQHandler
	void IWDG_t::IRQHandler() const {
		if (self[IWDGReg::SR].bitof(_IWDG_SR_EWIF_Pos)) {
			self[IWDGReg::EWCR].setof(_IWDG_EWCR_EWIC_Pos, true); // EWIC = 1 clears EWIF
			asserv(EwiCallback)();
		}
	}

	IWDG_t IWDG1(1);
	IWDG_t IWDG2(2);
#else
	IWDG_t IWDG1(1);
#endif

#if defined(_MCU_STM32H7x)
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

	WWDG_t WWDG1(1);
#endif
}

#if defined(_MCU_STM32H7x)
// vector table refers to this symbol; IRQ handler dispatches to WWDG1
void WWDG_IRQHandler(void) {
	uni::WWDG1.IRQHandler();
}
#endif

#endif
