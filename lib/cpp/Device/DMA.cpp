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

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
	#define _DMA_Counts 2

	static Request_t DMA1_Request_list[] = { Request_None,
		IRQ_DMA1_Channel1, IRQ_DMA1_Channel2, IRQ_DMA1_Channel3,
		IRQ_DMA1_Channel4, IRQ_DMA1_Channel5, IRQ_DMA1_Channel6, IRQ_DMA1_Channel7
	};// by channel id
	static Request_t DMA2_Request_list[] = { Request_None,
		IRQ_DMA2_Channel1, IRQ_DMA2_Channel2, IRQ_DMA2_Channel3,
		IRQ_DMA2_Channel4_5, IRQ_DMA2_Channel4_5
	};// by channel id

	static Request_t* DMAx_Requests_list[_DMA_Counts + 1] = {
		(Request_t*)0, DMA1_Request_list, DMA2_Request_list
	};

#endif
#if 0

#elif defined(_MCU_STM32F1x)
	
	static const uint32 _REFADDR_DMA[] = { nil,
		0x40020000, 0x40020400
	};
	
	static stduint RCC_DMAx_addrs[_DMA_Counts] = // 0.._DMA_Counts
	{
		_RCC_AHBENR_ADDR,_RCC_AHBENR_ADDR
	};
	static stduint RCC_DMAx_bitpos[_DMA_Counts] = // 0.._DMA_Counts
	{
		_RCC_AHBENR_POSI_ENCLK_DMA1,
		_RCC_AHBENR_POSI_ENCLK_DMA2,
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



#elif defined(_MCU_STM32F4x)

	static const uint32 _REFADDR_DMA[] = { nil,
		0x40026000, 0x40026400
	};

	static stduint RCC_DMAx_addrs[_DMA_Counts] = // 0.._DMA_Counts
	{
		_RCC_AHB1ENR_ADDR,_RCC_AHB1ENR_ADDR
	};
	static stduint RCC_DMAx_bitpos[_DMA_Counts] = // 0.._DMA_Counts
	{
		_RCC_AHB1ENR_POSI_ENCLK_DMA1,
		_RCC_AHB1ENR_POSI_ENCLK_DMA2,
	};

	bool DMAChannel::setMode(
		bool from_periph, bool to_periph, // DIR
		bool periph_inc, bool memory_inc, bool circular_mode,// Mode
		stduint periph_align, stduint memory_align, // Data Size
		byte priority) const
	{
		using namespace DMAReg;
		byte st = getParent().getID();
		DMA_t& sel = getParent().getParent();
		getParent().enAble(false);
		Reference cr = sel[CR[st]];
		cr.maset(25, 3, getID());// ChSel
		//{} cr.maset(23,2,);// MBURST
		//{} cr.maset(21, 2, );// PBURST
		//{} cr.setof(19, );// CT
		//{} cr.setof(18, );// DBM
		cr.maset(16, 2, priority);// PL
		//{} cr.setof(15,);// PINCOS
		cr.maset(13, 2, memory_align - 2);// MSIZE
		cr.maset(11, 2, periph_align - 2);// PSIZE
		cr.setof(10, memory_inc);// MINC
		cr.setof(9, periph_inc);// PINC
		cr.setof(8, circular_mode);// CIRC
		cr.maset(6, 2, (to_periph ? 1 : 0) + (from_periph ^ to_periph ? 0 : 2));// DIR
		//{} cr.setof(5, );// PFCTRL
		//{} cr.setof(4, );// TCIE
		//{} cr.setof(3, );// HTIE
		//{} cr.setof(2, );// TEIE
		//{} cr.setof(1, );// DMEIE
		//{} cr.setof(0, );// EN

		_TEMP;// DMA_FIFOMode = DMA_FIFOMode_Disable; 0
		// if(hdma->Init.FIFOMode == DMA_FIFOMODE_ENABLE) tmp |=  hdma->Init.MemBurst | hdma->Init.PeriphBurst;
		Reference fcr = sel[FCR[st]];
		fcr.setof(2, 0/*DMA_FIFOMode_Disable 0*/);
		if (fcr.bitof(2)) {
			_TEMP;// DMA_FIFOThreshold = DMA_FIFOThreshold_Full; 3
			setFIFOThreshold(DMAChannel::_Full);
			_TEMP;// DMA_MemoryBurst = DMA_MemoryBurst_Single; 0
			_TEMP;// DMA_PeripheralBurst = DMA_PeripheralBurst_Single; 0
			if (false /*MemBurst!=Single 0*/  /*&& DMA_CheckFifoParam*/) {
				return false;
			}
		}
		getParent().ClearInterruptFlags();
		return true;
	}

	void DMAChannel::setFIFOThreshold(DMAChannel::FIFOThreshold hold) const {
		using namespace DMAReg;
		byte st = getParent().getID();
		DMA_t& sel = getParent().getParent();
		Reference fcr = sel[FCR[st]];
		fcr.maset(0, 2, _IMM(hold));
	}



#endif

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)

	Reference DMA_t::operator[](DMAReg::DMARegType idx) {
		return Reference(_REFADDR_DMA[DMA_ID] + _IMMx4(idx));
	}

	bool DMA_t::enClock(bool ena) {
		Reference(RCC_DMAx_addrs[DMA_ID - 1]).setof(RCC_DMAx_bitpos[DMA_ID - 1], ena);
		if (ena != Reference(RCC_DMAx_addrs[DMA_ID - 1]).bitof(RCC_DMAx_bitpos[DMA_ID - 1]))
			return false;
		return true;
	}
	
	//

	DMA_t DMAr(0), DMA1(1), DMA2(2);

#endif
}
