// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Basic Direct Memory Access
// Codifiers: @dosconio: 20260816~;
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: RCC, NVIC
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

#include "../../../inc/cpp/Device/BDMA"
#include "../../../inc/cpp/Device/RCC/RCCAddress"
#include "../../../inc/cpp/Device/NVIC"
#include "../../../inc/cpp/Device/SysTick"
#include "../../../inc/cpp/Device/Interrupt/interrupt_tab.h"

namespace uni {
#if defined(_MCU_STM32H7x)

	// BDMA is in D3 domain; RCC_AHB4ENR.BDMAEN (bit21) gates BDMA + DMAMUX2 together.
	static const stduint _BDMA_ADDR = D3_AHB1PERIPH_BASE + 0x5400;
	static const stduint _DMAMUX2_ADDR = D3_AHB1PERIPH_BASE + 0x5800;
	#define _BDMA_TIMEOUT_VALUE 0x1FFFFFFUL

	static Request_t BDMA_Request_list[] = {
		IRQ_BDMA_Channel0, IRQ_BDMA_Channel1, IRQ_BDMA_Channel2, IRQ_BDMA_Channel3,
		IRQ_BDMA_Channel4, IRQ_BDMA_Channel5, IRQ_BDMA_Channel6, IRQ_BDMA_Channel7
	};

	Reference BDMA_t::operator[](BDMAReg::BDMARegType idx) {
		return Reference(_BDMA_ADDR + _IMMx4(idx));
	}

	bool BDMA_t::enClock(bool ena) {
		Reference(_RCC_AHB4ENR_ADDR).setof(21, ena);// BDMAEN gates BDMA + DMAMUX2
		return Reference(_RCC_AHB4ENR_ADDR).bitof(21) == ena;
	}

	bool BDMA_t::enAble(byte channel, bool ena) {
		using namespace BDMAReg;
		if (channel > 7) return false;
		self[(BDMARegType)(CCR1 + 5 * channel)].setof(_BDMA_CCR_POS_EN, ena);
		return true;
	}

	bool BDMA_t::setMode(byte channel, bool from_periph, bool to_periph,
		bool periph_inc, bool memory_inc, stduint periph_align, stduint memory_align,
		bool circular_mode, byte priority) {
		using namespace BDMAReg;
		if (channel > 7) return false;
		if (from_periph && to_periph) return false;
		if (!periph_align || periph_align > 4 || !memory_align || memory_align > 4) return false;
		stduint ccr = 0;
		if (from_periph ^ to_periph)
			ccr |= ((!from_periph) ? 1U : 0U) << _BDMA_CCR_POS_DIR;
		else
			ccr |= 1U << _BDMA_CCR_POS_MEM2MEM;
		if (periph_inc) ccr |= 1U << _BDMA_CCR_POS_PINC;
		if (memory_inc) ccr |= 1U << _BDMA_CCR_POS_MINC;
		memory_align >>= 1; periph_align >>= 1;
		ccr |= (memory_align << _BDMA_CCR_POS_MSIZE) | (periph_align << _BDMA_CCR_POS_PSIZE);
		if (circular_mode) ccr |= 1U << _BDMA_CCR_POS_CIRC;
		ccr |= (priority & 0x3U) << _BDMA_CCR_POS_PL;
		self[(BDMARegType)(CCR1 + 5 * channel)] = ccr;
		return true;
	}

	void BDMA_t::setTransfer(byte channel, pureptr_t dst_addr, pureptr_t src_addr, stduint leng) {
		using namespace BDMAReg;
		bool m2p = self[(BDMARegType)(CCR1 + 5 * channel)].bitof(_BDMA_CCR_POS_DIR);
		self[(BDMARegType)(CNDTR1 + 5 * channel)] = leng;
		self[(BDMARegType)(CPAR1 + 5 * channel)] = _IMM(m2p ? dst_addr : src_addr);
		self[(BDMARegType)(CMAR1 + 5 * channel)] = _IMM(m2p ? src_addr : dst_addr);
	}

	void BDMA_t::setRequest(byte channel, stduint request_id) {
		if (channel > 7) return;
		Reference(_DMAMUX2_ADDR + channel * 4) = request_id & 0xFF;
	}

	void BDMA_t::setInterruptSub(bool ena_total, byte channel) {
		using namespace BDMAReg;
		BDMARegType ccr = (BDMARegType)(CCR1 + 5 * channel);
		self[ccr].setof(_BDMA_CCR_POS_TCIE, ena_total);
		self[ccr].setof(_BDMA_CCR_POS_TEIE, ena_total);
		self[ccr].setof(_BDMA_CCR_POS_HTIE, ena_total && XferHalfCallback);
	}

	void BDMA_t::setInterruptPriority(byte channel, byte preempt, byte sub_priority) {
		if (channel > 7) return;
		NVIC.setPriority(BDMA_Request_list[channel], preempt, sub_priority);
	}

	void BDMA_t::enInterruptNVIC(byte channel, bool ena) {
		if (channel > 7) return;
		NVIC.setAble(BDMA_Request_list[channel], ena);
	}

	bool BDMA_t::Transfer(byte channel, pureptr_t dst_addr, pureptr_t src_addr, stduint leng, IOMethod method) {
		using namespace BDMAReg;
		if (channel > 7) return false;
		if (channelStates[channel] != _BDMA_STATE_READY) return false;
		channelStates[channel] = _BDMA_STATE_BUSY;
		self[IFCR].maset(channel * 4, 4, 0xF);
		setTransfer(channel, dst_addr, src_addr, leng);
		if (method == IOMethod::Rupt) {
			setInterruptSub(true, channel);
			enInterruptNVIC(channel, true);
			enAble(channel, true);
			return true;
		}
		// Loop: poll TCIF
		enAble(channel, true);
		uint64 tickstart = SysTick::getTick();
		while (!self[ISR].bitof(channel * 4 + 1)) {// TCIF
			if ((SysTick::getTick() - tickstart) > _BDMA_TIMEOUT_VALUE) {
				channelStates[channel] = _BDMA_STATE_TIMEOUT;
				enAble(channel, false);
				return false;
			}
		}
		self[IFCR].maset(channel * 4, 4, 0xF);
		channelStates[channel] = _BDMA_STATE_READY;
		if (XferCpltCallback) XferCpltCallback();
		return true;
	}

	bool BDMA_t::Abort(byte channel) {
		using namespace BDMAReg;
		if (channel > 7) return false;
		enAble(channel, false);
		self[IFCR].maset(channel * 4, 4, 0xF);
		channelStates[channel] = _BDMA_STATE_READY;
		return true;
	}

	bool BDMA_t::AbortRupt(byte channel) {
		using namespace BDMAReg;
		if (channel > 7) return false;
		enAble(channel, false);
		self[IFCR].maset(channel * 4, 4, 0xF);
		channelStates[channel] = _BDMA_STATE_READY;
		if (XferAbortCallback) XferAbortCallback();
		return true;
	}

	void BDMA_t::HandleIRQ(byte channel) {
		if (channel > 7) return;
		using namespace BDMAReg;
		stduint isr = self[ISR];
		stduint base = channel * 4;
		if (isr & (1U << (base + 3))) {// TEIF
			self[IFCR].maset(base, 4, 0xF);
			channelStates[channel] = _BDMA_STATE_ERROR;
			enAble(channel, false);
			if (XferErrorCallback) XferErrorCallback();
			return;
		}
		if (isr & (1U << (base + 2))) {// HTIF
			self[IFCR].setof(base + 2);
			if (XferHalfCallback) XferHalfCallback();
		}
		if (isr & (1U << (base + 1))) {// TCIF
			self[IFCR].maset(base, 4, 0xF);
			channelStates[channel] = _BDMA_STATE_READY;
			if (XferCpltCallback) XferCpltCallback();
		}
	}

	BDMA_t BDMA;

#endif
}
