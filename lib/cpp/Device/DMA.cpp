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
#include "../../../inc/cpp/Device/NVIC"
#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
#include "../../../inc/cpp/Device/SysTick"
#endif

namespace uni {

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
	#define _DMA_Counts 2
#endif
#if defined(_MCU_STM32F1x)

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
#elif defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
	// by stream id (0..7)
	static Request_t DMA1_Stream_Requests_list[] = {
		IRQ_DMA1_Stream0, IRQ_DMA1_Stream1, IRQ_DMA1_Stream2, IRQ_DMA1_Stream3,
		IRQ_DMA1_Stream4, IRQ_DMA1_Stream5, IRQ_DMA1_Stream6, IRQ_DMA1_Stream7
	};
	static Request_t DMA2_Stream_Requests_list[] = {
		IRQ_DMA2_Stream0, IRQ_DMA2_Stream1, IRQ_DMA2_Stream2, IRQ_DMA2_Stream3,
		IRQ_DMA2_Stream4, IRQ_DMA2_Stream5, IRQ_DMA2_Stream6, IRQ_DMA2_Stream7
	};
	static Request_t* DMAx_Stream_Requests_list[_DMA_Counts + 1] = {
		(Request_t*)0, DMA1_Stream_Requests_list, DMA2_Stream_Requests_list
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



#elif defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)

#if defined(_MCU_STM32F4x)
	static const uint32 _REFADDR_DMA[] = { nil,
		0x40026000, 0x40026400
	};
#elif defined(_MCU_STM32H7x)
	static const uint32 _REFADDR_DMA[] = { nil,
		0x40020000, 0x40020400
	};
	// DMAMUX1 routes peripheral requests to DMA1/DMA2 streams (replaces F4 CHSEL)
	#define _DMAMUX1_BASE 0x40020800
#endif

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
		// widths are encoded 2/3/4 = byte/half/word; reject reserved DIR (periph->periph)
		if (periph_align < 2 || periph_align > 4 ||
			memory_align < 2 || memory_align > 4) return false;
		if (from_periph && to_periph) return false;
		getParent().enAble(false);
		Reference cr = sel[CR[st]];
#if defined(_MCU_STM32F4x)
		cr.maset(_DMA_SxCR_POS_CHSEL, 3, getID());// ChSel - F4 fixed peripheral mapping
#elif defined(_MCU_STM32H7x)
		// H7: request routing is via DMAMUX1 (call setRequest); CHSEL is left default
#endif
		//{} cr.maset(_DMA_SxCR_POS_MBURST, 2, );// MBURST
		//{} cr.maset(_DMA_SxCR_POS_PBURST, 2, );// PBURST
		//{} cr.setof(_DMA_SxCR_POS_CT, );// CT
		//{} cr.setof(_DMA_SxCR_POS_DBM, );// DBM
		cr.maset(_DMA_SxCR_POS_PL, 2, priority);// PL
		//{} cr.setof(_DMA_SxCR_POS_PINCOS, );// PINCOS
		cr.maset(_DMA_SxCR_POS_MSIZE, 2, memory_align - 2);// MSIZE
		cr.maset(_DMA_SxCR_POS_PSIZE, 2, periph_align - 2);// PSIZE
		cr.setof(_DMA_SxCR_POS_MINC, memory_inc);// MINC
		cr.setof(_DMA_SxCR_POS_PINC, periph_inc);// PINC
		cr.setof(_DMA_SxCR_POS_CIRC, circular_mode);// CIRC
		cr.maset(_DMA_SxCR_POS_DIR, 2, (to_periph ? 1 : 0) + (from_periph ^ to_periph ? 0 : 2));// DIR
		//{} cr.setof(_DMA_SxCR_POS_PFCTRL, );// PFCTRL
		// IE bits are set by setInterruptSub / enInterrupt

		Reference fcr = sel[FCR[st]];
		fcr.setof(_DMA_SxFCR_POS_DMDIS, 0/*DMA_FIFOMode_Disable*/);
		if (fcr.bitof(_DMA_SxFCR_POS_DMDIS)) {
			//{TODO} DMA_FIFOThreshold + MemBurst/PeriphBurst + DMA_CheckFifoParam
			setFIFOThreshold(DMAChannel::_Full);
		}
		getParent().ClearInterruptFlags();
		return true;
	}

	void DMAChannel::setFIFOThreshold(DMAChannel::FIFOThreshold hold) const {
		using namespace DMAReg;
		byte st = getParent().getID();
		DMA_t& sel = getParent().getParent();
		Reference fcr = sel[FCR[st]];
		fcr.maset(_DMA_SxFCR_POS_FTH, 2, _IMM(hold));
	}

	void DMAStream::setRequest(stduint request_id) const {
		using namespace DMAReg;
		DMA_t& sel = getParent();
		byte st = getID();
#if defined(_MCU_STM32F4x)
		sel[CR[st]].maset(_DMA_SxCR_POS_CHSEL, 3, request_id);
#elif defined(_MCU_STM32H7x)
		// DMA1 Stream N -> DMAMUX1 Channel N ; DMA2 Stream N -> DMAMUX1 Channel N+8
		byte mux_index = st;
		if (sel.getID() == 2) mux_index += 8;
		Reference(_DMAMUX1_BASE + mux_index * 4) = request_id;
#else
		(void)st; (void)request_id;
#endif
	}

	void DMAStream::setInterruptPriority(byte preempt, byte sub_priority) const {
		DMA_t& sel = getParent();
		NVIC.setPriority(DMAx_Stream_Requests_list[sel.getID()][getID()], preempt, sub_priority);
	}

	void DMAStream::enInterruptNVIC(bool ena) const {
		DMA_t& sel = getParent();
		NVIC.setAble(DMAx_Stream_Requests_list[sel.getID()][getID()], ena);
	}

	bool DMAStream::Transfer(pureptr_t dst_addr, pureptr_t src_addr, stduint leng, IOMethod method) const {
		using namespace DMAReg;
		DMA_t& sel = getParent();
		byte st = getID();
		if (sel.streamStates[st] != _DMA_STATE_READY) return false;

		if (method == IOMethod::Rupt) {
			// aka HAL_DMA_Start_IT
			enAble(false);
			setTransfer(dst_addr, src_addr, leng);
			setInterruptSub(true);
			enInterruptNVIC(true);
			sel.streamStates[st] = _DMA_STATE_BUSY;
			enAble(true);
			return true;
		}

		if (method != IOMethod::Loop) return false; // DMA sub-mode N/A for DMA itself

		// IOMethod::Loop: aka HAL_DMA_Start + HAL_DMA_PollForTransfer
		if (sel[CR[st]].bitof(_DMA_SxCR_POS_CIRC)) {
			sel.streamErrors[st] = _DMA_ERROR_NOT_SUPPORTED;
			return false;
		}
		enAble(false);
		setTransfer(dst_addr, src_addr, leng);
		sel.streamStates[st] = _DMA_STATE_BUSY;
		enAble(true);

		uint64 tickstart = SysTick::getTick();
		byte posi = flagBase();
		DMARegType isr_reg = (st & 0b100) ? HISR : LISR;
		while (!(sel[isr_reg] & (1U << (posi + _DMA_SxFLAG_POS_TCIF)))) {
			if (sel[isr_reg] & (1U << (posi + _DMA_SxFLAG_POS_TEIF))) {
				sel.streamErrors[st] = _DMA_ERROR_TE;
				sel.streamStates[st] = _DMA_STATE_ERROR;
				ClearInterruptFlags();
				return false;
			}
			if ((SysTick::getTick() - tickstart) > DMA_TIMEOUT_VALUE) {
				sel.streamErrors[st] = _DMA_ERROR_TIMEOUT;
				Abort();
				return false;
			}
		}
		ClearInterruptFlags();
		sel.streamStates[st] = _DMA_STATE_READY;
		return true;
	}

	bool DMAStream::Abort() const {
		using namespace DMAReg;
		DMA_t& sel = getParent();
		byte st = getID();
		if (sel.streamStates[st] != _DMA_STATE_BUSY) {
			sel.streamErrors[st] = _DMA_ERROR_NO_XFER;
			return false;
		}
		// disable all interrupts
		Reference cr = sel[CR[st]];
		cr.setof(_DMA_SxCR_POS_TCIE, false);
		cr.setof(_DMA_SxCR_POS_TEIE, false);
		cr.setof(_DMA_SxCR_POS_DMEIE, false);
		cr.setof(_DMA_SxCR_POS_HTIE, false);
		sel[FCR[st]].setof(_DMA_SxFCR_POS_FEIE, false);
		// disable stream
		enAble(false);
		// wait for EN=0 with timeout
		uint64 tickstart = SysTick::getTick();
		while (sel[CR[st]].bitof(_DMA_SxCR_POS_EN)) {
			if ((SysTick::getTick() - tickstart) > DMA_TIMEOUT_VALUE) {
				sel.streamErrors[st] = _DMA_ERROR_TIMEOUT;
				sel.streamStates[st] = _DMA_STATE_ERROR;
				return false;
			}
		}
		ClearInterruptFlags();
		sel.streamStates[st] = _DMA_STATE_READY;
		return true;
	}

	bool DMAStream::AbortRupt() const {
		DMA_t& sel = getParent();
		byte st = getID();
		if (sel.streamStates[st] != _DMA_STATE_BUSY) {
			sel.streamErrors[st] = _DMA_ERROR_NO_XFER;
			return false;
		}
		// set abort state; IRQ handler completes cleanup on next TCIF
		sel.streamStates[st] = _DMA_STATE_ABORT;
		enAble(false);
		return true;
	}



#endif

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)

	Reference DMA_t::operator[](DMAReg::DMARegType idx) {
		return Reference(_REFADDR_DMA[DMA_ID] + _IMMx4(idx));
	}

	bool DMA_t::enClock(bool ena) {
		Reference(RCC_DMAx_addrs[DMA_ID - 1]).setof(RCC_DMAx_bitpos[DMA_ID - 1], ena);
	#if defined(_MCU_STM32H7x)
		// DMAMUX1 is the request router for both DMA1 and DMA2; clock it together
		Reference(_RCC_AHB1ENR_ADDR).setof(_RCC_AHB1ENR_POSI_ENCLK_DMAMUX1, ena);
	#endif
		if (ena != Reference(RCC_DMAx_addrs[DMA_ID - 1]).bitof(RCC_DMAx_bitpos[DMA_ID - 1]))
			return false;
		return true;
	}


	DMA_t DMAr(0), DMA1(1), DMA2(2);

#endif
}
