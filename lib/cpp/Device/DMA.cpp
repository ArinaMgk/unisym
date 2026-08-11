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
	// DMAMUX1 Channel Configuration Register (DMAMUX_CxCR) bit layout — H7 only.
	// [7:0] DMAREQ_ID : DMA request ID (written by setRequest)
	// [8]   SOIE      : Synchronization overrun interrupt enable
	// [9]   EGE       : Event generation enable
	// [16]  SE        : Synchronization enable
	// [18:17] SPOL    : Synchronization polarity (0: no event, 1: rising, 2: falling)
	// [24:19] NBREQ   : Number of DMA requests authorized per sync event (N-1 encoded)
	// [29:25] SYNC_ID : Synchronization input signal ID
	#define _DMAMUX_CxCR_SOIE    0x00000100U
	#define _DMAMUX_CxCR_EGE     0x00000200U
	#define _DMAMUX_CxCR_SE      0x00010000U
	#define _DMAMUX_CxCR_SPOL_Pos 17U
	#define _DMAMUX_CxCR_NBREQ_Pos 19U
	#define _DMAMUX_CxCR_SYNC_ID_Pos 25U
	// DMAMUX1 Request Generator Register (DMAMUX_RGxCR) bit layout — H7 only.
	// [8]   OIE       : Request generator overrun interrupt enable
	// [16]  GE        : Request generator enable
	// [18:17] GPOL    : Request generator polarity (0: no event, 1: rising, 2: falling)
	// [24:19] GNBREQ  : Number of requests to generate per event (N-1 encoded)
	// [31:25] SIGNAL_ID : Request generator input signal ID
	#define _DMAMUX_RGxCR_OIE      0x00000100U
	#define _DMAMUX_RGxCR_GE       0x00010000U
	#define _DMAMUX_RGxCR_GPOL_Pos 17U
	#define _DMAMUX_RGxCR_NBREQ_Pos 19U
	#define _DMAMUX_RGxCR_SIGNAL_ID_Pos 25U
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

	// AKA HAL_DMAEx_MultiBufferStart / HAL_DMAEx_MultiBufferStart_IT
	// Memory-to-memory transfer not supported in double buffering mode.
	// When Double Buffer mode is enabled, the transfer is circular by default.
	bool DMAStream::MultiBufferTransfer(pureptr_t dst_addr, pureptr_t src_addr, pureptr_t second_mem, stduint leng, IOMethod method) const {
		using namespace DMAReg;
		DMA_t& sel = getParent();
		byte st = getID();
		// Memory-to-memory transfer not supported in double buffering mode
		if (sel[CR[st]].masof(_DMA_SxCR_POS_DIR, 2) == 2) {
			sel.streamErrors[st] = _DMA_ERROR_NOT_SUPPORTED;
			return false;
		}
		if (sel.streamStates[st] != _DMA_STATE_READY) {
			sel.streamErrors[st] = _DMA_ERROR_BUSY;
			return false;
		}
		sel.streamStates[st] = _DMA_STATE_BUSY;
		sel.streamErrors[st] = _DMA_ERROR_NONE;
		// Enable the double buffer mode
		sel[CR[st]].setof(_DMA_SxCR_POS_DBM, true);
		// Configure DMA Stream second memory address
		sel[M1AR[st]] = _IMM(second_mem);
		// Configure the source, destination address and the data length
		sel[NDTR[st]] = leng;
		// Use direction bit from existing CR config (DIR set by DMAChannel::setMode)
		bool is_m2p = 1 == sel[CR[st]].masof(_DMA_SxCR_POS_DIR, 2);
		sel[PAR[st]] = _IMM(is_m2p ? dst_addr : src_addr);
		sel[M0AR[st]] = _IMM(is_m2p ? src_addr : dst_addr);
		// Clear all stream flags
		ClearInterruptFlags();
		if (method == IOMethod::Rupt) {
			// Enable common interrupts (TCIE | TEIE | DMEIE)
			setInterruptSub(true);
			// Double buffer: also enable HT if half callbacks are registered
			if (sel.XferHalfCallback || sel.XferM1HalfCpltCallback)
				sel[CR[st]].setof(_DMA_SxCR_POS_HTIE, true);
			enInterruptNVIC(true);
		}
		// Enable the stream
		enAble(true);
		return true;
	}

	// AKA HAL_DMAEx_ChangeMemory: switch M0AR or M1AR on the fly
	void DMAStream::ChangeMemory(pureptr_t addr, MemorySel mem) const {
		using namespace DMAReg;
		DMA_t& sel = getParent();
		byte st = getID();
		if (mem == MemorySel::Memory0) {
			sel[M0AR[st]] = _IMM(addr);
		} else {
			sel[M1AR[st]] = _IMM(addr);
		}
	}

#if defined(_MCU_STM32H7x)
	// AKA HAL_DMAEx_ConfigMuxSync: configure DMAMUX synchronization for this stream
	// Clear SE+EGE before applying new config; keeps existing DMAREQ_ID.
	// DMAMUX_CxCR [7:0] DMAREQ_ID, [8] SOIE, [9] EGE, [16] SE, [18:17] SPOL, [24:19] NBREQ, [29:25] SYNC_ID
	bool DMAStream::setMuxSync(stduint signal_id, MuxPolarity pol, bool ena, bool event_ena, stduint request_num) const {
		DMA_t& sel = getParent();
		byte st = getID();
		byte mux_idx = st;
		if (sel.getID() == 2) mux_idx += 8;
		stduint ccr_addr = _DMAMUX1_BASE + mux_idx * 4;
		if (sel.streamStates[st] != _DMA_STATE_READY) {
			sel.streamErrors[st] = _DMA_ERROR_BUSY;
			return false;
		}
		// Disable synchronization and event generation before applying new config
		Reference ccr = Reference(ccr_addr);
		ccr &= ~(_DMAMUX_CxCR_SE | _DMAMUX_CxCR_EGE);
		if (ena) {
			stduint new_val = ((stduint)ccr & 0xFFU)                                 // keep DMAREQ_ID[7:0]
				| (signal_id << _DMAMUX_CxCR_SYNC_ID_Pos)                            // SYNC_ID[4:0] @ bit 25
				| ((request_num - 1U) << _DMAMUX_CxCR_NBREQ_Pos)                     // NBREQ @ bit 19
				| (((stduint)pol) << _DMAMUX_CxCR_SPOL_Pos)                          // SPOL @ bit 17
				| _DMAMUX_CxCR_SE;                                                   // enable sync
			if (event_ena) new_val |= _DMAMUX_CxCR_EGE;
			ccr = new_val;
		}
		return true;
	}

	// AKA HAL_DMAEx_ConfigMuxRequestGenerator: configure DMAMUX request generator
	// DMAMUX_RGxCR: [8] OIE, [16] GE, [18:17] GPOL, [24:19] GNBREQ, [31:25] SIGNAL_ID
	// Note: DMAMUX1 has 8 request generators (indices follow the stream index 0..7);
	//       a request generator is typically used by DMA2 streams.
	bool DMAStream::ConfigRequestGenerator(stduint signal_id, MuxPolarity pol, stduint request_num) const {
		DMA_t& sel = getParent();
		byte st = getID();
		stduint rgcr_addr = _DMAMUX1_BASE + 0x100U + st * 0x04U;
		Reference rgcr = Reference(rgcr_addr);
		// RequestGenerator must be disabled before configuring (GE bit must be 0)
		if (rgcr.bitof(16)) {
			sel.streamErrors[st] = _DMA_ERROR_BUSY;
			return false;
		}
		rgcr = (signal_id << _DMAMUX_RGxCR_SIGNAL_ID_Pos)
			| ((request_num - 1U) << _DMAMUX_RGxCR_NBREQ_Pos)
			| (((stduint)pol) << _DMAMUX_RGxCR_GPOL_Pos);
		return true;
	}

	// AKA HAL_DMAEx_EnableMuxRequestGenerator / DisableMuxRequestGenerator
	bool DMAStream::EnableRequestGenerator(bool ena) const {
		DMA_t& sel = getParent();
		byte st = getID();
		stduint rgcr_addr = _DMAMUX1_BASE + 0x100U + st * 0x04U;
		Reference rgcr = Reference(rgcr_addr);
		if (sel.streamStates[st] == _DMA_STATE_RESET)
			return false; // stream not initialized
		rgcr.setof(16, ena); // GE bit
		return true;
	}

	// AKA HAL_DMAEx_MUX_IRQHandler: handle DMAMUX synchronization / request-generator overrun
	// DMAMUX1 register layout (see RM0433):
	//   CCR[N]    = 0x100 + 0x4*N        (N=0..15, channel configuration; N=8+st for DMA2)
	//   CSR       = 0x080, CFR = 0x084   (channel status / status-clear, bit N = channel N)
	//   RGCR[N]   = 0x100 + 0x4*N        (N=0..7,  request generator configuration)
	//   RGSR      = 0x140, RGCFR = 0x144 (generator status / status-clear, bit N = generator N)
	void DMAStream::HandleMuxIRQ(void) const {
		DMA_t& sel = getParent();
		byte st = getID();
		byte mux_idx = st;
		if (sel.getID() == 2) mux_idx += 8;

		// Check DMAMUX synchronization overrun for this stream's channel
		Reference csr = Reference(_DMAMUX1_BASE + 0x080U);
		if (csr.bitof(mux_idx)) {
			// Disable sync overrun interrupt
			stduint ccr_addr = _DMAMUX1_BASE + mux_idx * 4;
			Reference(ccr_addr) &= ~_DMAMUX_CxCR_SOIE;
			// Clear sync overrun flag (write 1 to CFR bit)
			Reference(_DMAMUX1_BASE + 0x084U).setof(mux_idx, true);
			sel.streamErrors[st] |= _DMA_ERROR_SYNC;
			asserv(sel.XferErrorCallback)();
		}

		// Check request generator overrun (generators follow stream index 0..7)
		Reference rgsr = Reference(_DMAMUX1_BASE + 0x140U);
		if (rgsr.bitof(st)) {
			// Disable request generator overrun interrupt
			Reference rgcr = Reference(_DMAMUX1_BASE + 0x100U + st * 0x04U);
			rgcr &= ~_DMAMUX_RGxCR_OIE;
			// Clear request generator overrun flag (write 1 to RGCFR bit)
			Reference(_DMAMUX1_BASE + 0x144U).setof(st, true);
			sel.streamErrors[st] |= _DMA_ERROR_REQGEN;
			asserv(sel.XferErrorCallback)();
		}
	}
#endif

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
