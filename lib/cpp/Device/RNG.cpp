// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Random Number Generator, RNG
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

#include "../../../inc/c/random.h"

namespace uni {
#if defined(_MCU_STM32H7x)
	_ESYM_C void RNG_IRQHandler(void);
	_ESYM_C extern Handler_t FUNC_RNG[1];

	#define _RNG_TIMEOUT_VALUE 2U

	RNG RNG1;

	// AKA HAL_RNG_Init
	bool RNG::setMode() {
		if (State == RNGState::Reset) {
			enClock();
			State = RNGState::Busy;
			// AKA MODIFY_REG(CR, RNG_CR_CED, Init.ClockErrorDetection)
			self[RNGReg::CR].setof(_RNG_CR_POS_CED, clock_error_detection);
			enAble(true);// __HAL_RNG_ENABLE
			State = RNGState::Ready;
			return true;
		}
		return false;
	}

	// AKA HAL_RNG_DeInit
	void RNG::canMode() {
		// CLEAR_BIT(CR, IE | RNGEN | CED)
		self[RNGReg::CR].rstof(_RNG_CR_POS_IE);
		self[RNGReg::CR].rstof(_RNG_CR_POS_RNGEN);
		self[RNGReg::CR].rstof(_RNG_CR_POS_CED);
		// CLEAR_BIT(SR, CEIS | SEIS)
		self[RNGReg::SR].rstof(_RNG_SR_POS_CEIS);
		self[RNGReg::SR].rstof(_RNG_SR_POS_SECS);
		enClock(false);
		State = RNGState::Reset;
	}

	// AKA __HAL_RNG_ENABLE / DISABLE (CR.RNGEN)
	void RNG::enAble(bool ena) {
		self[RNGReg::CR].setof(_RNG_CR_POS_RNGEN, ena);
	}

	// RCC AHB2ENR.RNGEN (bit 6)
	void RNG::enClock(bool ena) {
		Reference(_RCC_AHB2ENR_ADDR).setof(6, ena);
	}

	// AKA HAL_RNG_GenerateRandomNumber / _IT
	bool RNG::Generate(stduint& random32bit, IOMethod method) {
		if (State != RNGState::Ready) return false;
		if (method == IOMethod::Loop) {
			State = RNGState::Busy;
			uint64 tickstart = SysTick::getTick();
			// wait DRDY (data register valid); reading DR clears it
			while (!self[RNGReg::SR].bitof(_RNG_SR_POS_DRDY)) {
				if ((SysTick::getTick() - tickstart) > _RNG_TIMEOUT_VALUE) {
					State = RNGState::Error;// HAL: timeout sets ERROR, no callback
					return false;
				}
			}
			RandomNumber = self[RNGReg::DR];
			random32bit = RandomNumber;
			State = RNGState::Ready;
			return true;
		}
		// IOMethod::Rupt: arm RNG interrupt (DRDY/CEI/SEI), data arrives in RNG_IRQHandler
		if (method == IOMethod::Rupt) {
			State = RNGState::Busy;
			self[RNGReg::CR].setof(_RNG_CR_POS_IE);// __HAL_RNG_ENABLE_IT
			return true;
		}
		return false;
	}

	// ---- RuptTrait (NVIC + IRQ_RNG) ----
	void RNG::setInterrupt(Handler_t f) const {
		FUNC_RNG[0] = f;
	}
	void RNG::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(IRQ_RNG, preempt, sub_priority);
	}
	void RNG::enInterrupt(bool enable) const {
		NVIC.setAble(IRQ_RNG, enable);
	}

#endif // _MCU_STM32H7x
}
