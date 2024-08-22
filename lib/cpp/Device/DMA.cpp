// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Direct Memory Access
// Codifiers: @dosconio: 20240712~;
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: GPIO and other DMA Channnels
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


#include "../../../inc/cpp/Device/DMA"
#include "../../../inc/cpp/Device/RCC/RCCAddress"
#include "../../../inc/cpp/Device/RCC/RCCClock"

namespace uni {
	#if 0

	#elif defined(_MCU_STM32F1x)
	#define _DMA_Counts 3

	static Request_t DMA1_Request_list[] = {
		(Request_t)0, IRQ_DMA1_Channel1, IRQ_DMA1_Channel2, IRQ_DMA1_Channel3,
		IRQ_DMA1_Channel4, IRQ_DMA1_Channel5, IRQ_DMA1_Channel6, IRQ_DMA1_Channel7
	};// by channel id
	static Request_t DMA2_Request_list[] = {
		(Request_t)0, IRQ_DMA2_Channel1, IRQ_DMA2_Channel2, IRQ_DMA2_Channel3,
		IRQ_DMA2_Channel4_5, IRQ_DMA2_Channel4_5
	};// by channel id
	static Request_t* DMAx_Requests_list[_DMA_Counts + 1] = {
		(Request_t*)0, DMA1_Request_list, DMA2_Request_list
	};

	bool DMA_t::ExistChannel(byte channel) {
		if (!channel) return false;
		if (DMA_ID == 1) {
			if (channel > 7) return false;
			else return true;
		}
		else if (DMA_ID == 2) {
			if (channel > 5) return false;
			else return true;
		} // only for : STM32F101xE || STM32F101xG || STM32F103xE || STM32F103xG || STM32F100xE || STM32F105xC || STM32F107xC
		else return false;
	}

	
	static stduint RCC_DMAx_addrs[_DMA_Counts] = // 0.._DMA_Counts
	{
		_RCC_AHBENR_ADDR,_RCC_AHBENR_ADDR,0
	};
	static stduint RCC_DMAx_bitpos[_DMA_Counts] = // 0.._DMA_Counts
	{
		_RCC_AHBENR_POSI_ENCLK_DMA1,
		_RCC_AHBENR_POSI_ENCLK_DMA2,
	};
	bool DMA_t::enClock(bool ena) {
		Reference(RCC_DMAx_addrs[DMA_ID - 1]).setof(RCC_DMAx_bitpos[DMA_ID - 1], ena);
		if (ena != Reference(RCC_DMAx_addrs[DMA_ID - 1]).bitof(RCC_DMAx_bitpos[DMA_ID - 1]))
			return false;
		return true;
	}

	bool DMA_t::setMode(byte channel, bool from_periph_or_memory, bool to_periph_or_memory, bool periph_inc, bool memory_inc, stduint periph_align, stduint memory_align, bool circular_mode, byte priority) {
		using namespace DMAReg;
		if (!ExistChannel(channel)) return false;
		stduint ccr = self[CCRx[channel]] & 0xF; // 15 bits, keep EN and IE
		if (from_periph_or_memory && to_periph_or_memory) return false;
		else if (from_periph_or_memory ^ to_periph_or_memory)
			BitSev(ccr, _DMA_CCRx_POS_DIR, (!from_periph_or_memory));
		else BitSet(ccr, _DMA_CCRx_POS_M2M);
		m2p = !from_periph_or_memory && to_periph_or_memory;
		if (periph_inc) BitSet(ccr, _DMA_CCRx_POS_PINC);
		if (memory_inc) BitSet(ccr, _DMA_CCRx_POS_MINC);
		if (!periph_align || periph_align > 4 ||
			!memory_align || memory_align > 4) return false;
		memory_align >>= 1; periph_align >>= 1;
		// above leave 0b, 1b, 1b, 10b
		// treat 3 as 2, i.e. word, uint16
		ccr |= (memory_align << _DMA_CCRx_POS_MSIZE) | (periph_align << _DMA_CCRx_POS_PSIZE);
		if (circular_mode) BitSet(ccr, _DMA_CCRx_POS_CIRC);
		ccr |= (priority & 0x3) << _DMA_CCRx_POS_PL;
		self[CCRx[channel]] = ccr;
		return true;
	}

	void DMA_t::setInterruptPriority(byte channel, byte preempt, byte sub_priority) {
		if (!ExistChannel(channel)) return;
		NVIC.setPriority(
			DMAx_Requests_list[DMA_ID][channel], preempt, sub_priority);
	}

	//

	DMA_t DMAr(0,0), DMA1(0x40020000, 1), DMA2(0x40020400, 2);

	#endif
}
