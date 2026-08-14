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
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
#include "../../../inc/cpp/Device/NVIC"
#endif
#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
#include "../../../inc/cpp/Device/SysTick"
#endif
#if defined(_MPU_STM32MP13)
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../inc/cpp/Device/RCC/RCC"
#include "../../../inc/cpp/interrupt"
#include "../../../inc/c/driver/interrupt/GIC.h"
#endif

namespace uni {

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
	#define _DMA_Counts 2
#elif defined(_MPU_STM32MP13)
	#define _DMA_Counts 3
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
#elif defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	// by stream id (0..7)
	static Request_t DMA1_Stream_Requests_list[] = {
		IRQ_DMA1_Stream0, IRQ_DMA1_Stream1, IRQ_DMA1_Stream2, IRQ_DMA1_Stream3,
		IRQ_DMA1_Stream4, IRQ_DMA1_Stream5, IRQ_DMA1_Stream6, IRQ_DMA1_Stream7
	};
	static Request_t DMA2_Stream_Requests_list[] = {
		IRQ_DMA2_Stream0, IRQ_DMA2_Stream1, IRQ_DMA2_Stream2, IRQ_DMA2_Stream3,
		IRQ_DMA2_Stream4, IRQ_DMA2_Stream5, IRQ_DMA2_Stream6, IRQ_DMA2_Stream7
	};
#if defined(_MPU_STM32MP13)
	static Request_t DMA3_Stream_Requests_list[] = {
		IRQ_DMA3_Stream0, IRQ_DMA3_Stream1, IRQ_DMA3_Stream2, IRQ_DMA3_Stream3,
		IRQ_DMA3_Stream4, IRQ_DMA3_Stream5, IRQ_DMA3_Stream6, IRQ_DMA3_Stream7
	};
#endif
	static Request_t* DMAx_Stream_Requests_list[_DMA_Counts + 1] = {
		(Request_t*)0, DMA1_Stream_Requests_list, DMA2_Stream_Requests_list
#if defined(_MPU_STM32MP13)
		, DMA3_Stream_Requests_list
#endif
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



#elif defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

#if defined(_MCU_STM32F4x)
	static const uint32 _REFADDR_DMA[] = { nil,
		0x40026000, 0x40026400
	};
#elif defined(_MCU_STM32H7x)
	static const uint32 _REFADDR_DMA[] = { nil,
		0x40020000, 0x40020400
	};
#elif defined(_MPU_STM32MP13)
	static const uint32 _REFADDR_DMA[] = { nil,
		AHB2_PERIPH_BASE + 0x0000,  // DMA1
		AHB2_PERIPH_BASE + 0x1000,  // DMA2
		AHB2_PERIPH_BASE + 0x5000   // DMA3
	};
#endif

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	// DMAMUX routes peripheral requests to DMA streams (replaces F4 CHSEL)
	#if defined(_MCU_STM32H7x)
		#define _DMAMUX1_BASE 0x40020800
	#elif defined(_MPU_STM32MP13)
		#define _DMAMUX1_BASE (AHB2_PERIPH_BASE + 0x2000)
		#define _DMAMUX2_BASE (AHB2_PERIPH_BASE + 0x6000)
	#endif
	// DMAMUX Channel Configuration Register (DMAMUX_CxCR) bit layout — H7/MP13.
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
	// DMAMUX Request Generator Register (DMAMUX_RGxCR) bit layout — H7/MP13.
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

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	// DMAMUX instance that routes requests for a given DMA controller.
	//   H7:   both DMA1/DMA2 use DMAMUX1.
	//   MP13: DMA1/DMA2 use DMAMUX1, DMA3 uses DMAMUX2.
	static stduint _DMAMUX_Base(byte dma_id) {
#if defined(_MPU_STM32MP13)
		if (dma_id == 3) return _DMAMUX2_BASE;
#else
		(void)dma_id;
#endif
		return _DMAMUX1_BASE;
	}
	// DMAMUX base + channel index for a DMA stream.
	//   DMA1 Stream N -> DMAMUX1 Channel N ; DMA2 Stream N -> DMAMUX1 Channel N+8
	//   DMA3 Stream N -> DMAMUX2 Channel N (MP13 only)
	static stduint _DMAMUX_StreamSel(byte dma_id, byte st, byte& mux_index) {
		mux_index = st;
		if (dma_id == 2) mux_index += 8;
		return _DMAMUX_Base(dma_id);
	}
#endif

#if defined(_MPU_STM32MP13)
	// Request generator index = DMAMUX request id - 1 (request id 1..8 -> generator 0..7).
	static bool _DMAMUX_UseReqGen(const DMA_t& sel, byte st, byte& rg_index) {
		byte req = sel.streamRequests[st];
		if (req < 1 || req > 8) return false;
		rg_index = req - 1;
		return true;
	}
	// aka HAL_DMA_Start_IT DMAMUX part: enable sync/request-gen overrun interrupts if configured
	static void _DMAMUX_EnableOverrunIT(const DMA_t& sel, byte st) {
		byte mux_idx;
		stduint mux_base = _DMAMUX_StreamSel(sel.getID(), st, mux_idx);
		Reference ccr = Reference(mux_base + mux_idx * 4);
		if (ccr & _DMAMUX_CxCR_SE) ccr |= _DMAMUX_CxCR_SOIE;
		byte rg_index;
		if (_DMAMUX_UseReqGen(sel, st, rg_index))
			Reference(mux_base + 0x100U + rg_index * 4) |= _DMAMUX_RGxCR_OIE;
	}
	// aka HAL_DMA_Abort DMAMUX part: disable overrun interrupts + clear flags
	static void _DMAMUX_DisableOverrunIT(const DMA_t& sel, byte st) {
		byte mux_idx;
		stduint mux_base = _DMAMUX_StreamSel(sel.getID(), st, mux_idx);
		Reference(mux_base + mux_idx * 4) &= ~_DMAMUX_CxCR_SOIE;
		Reference(mux_base + 0x084U).setof(mux_idx, true);
		byte rg_index;
		if (_DMAMUX_UseReqGen(sel, st, rg_index)) {
			Reference(mux_base + 0x100U + rg_index * 4) &= ~_DMAMUX_RGxCR_OIE;
			Reference(mux_base + 0x144U).setof(rg_index, true);
		}
	}
	// aka DMA_SetConfig DMAMUX part: clear overrun flags before a transfer
	static void _DMAMUX_ClearOverrun(const DMA_t& sel, byte st) {
		byte mux_idx;
		stduint mux_base = _DMAMUX_StreamSel(sel.getID(), st, mux_idx);
		Reference(mux_base + 0x084U).setof(mux_idx, true);
		byte rg_index;
		if (_DMAMUX_UseReqGen(sel, st, rg_index))
			Reference(mux_base + 0x144U).setof(rg_index, true);
	}
#endif

#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)
	static stduint RCC_DMAx_addrs[_DMA_Counts] = // 0.._DMA_Counts
	{
		_RCC_AHB1ENR_ADDR,_RCC_AHB1ENR_ADDR
	};
	static stduint RCC_DMAx_bitpos[_DMA_Counts] = // 0.._DMA_Counts
	{
		_RCC_AHB1ENR_POSI_ENCLK_DMA1,
		_RCC_AHB1ENR_POSI_ENCLK_DMA2,
	};
#endif

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
		cr.maset(_DMA_SxCR_POS_MBURST, 2, 0);// MBURST - clear (aka HAL_DMA_Init)
		cr.maset(_DMA_SxCR_POS_PBURST, 2, 0);// PBURST - clear (aka HAL_DMA_Init)
		cr.setof(_DMA_SxCR_POS_CT, false);// CT - clear (aka HAL_DMA_Init)
		cr.setof(_DMA_SxCR_POS_DBM, false);// DBM - clear (aka HAL_DMA_Init)
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
		fcr.maset(_DMA_SxFCR_POS_FTH, 2, 0);// FTH - clear (aka HAL_DMA_Init)
		if (fcr.bitof(_DMA_SxFCR_POS_DMDIS)) {
			//{TODO} DMA_FIFOThreshold + MemBurst/PeriphBurst + DMA_CheckFifoParam
			setFIFOThreshold(DMAChannel::_Full);
		}
		getParent().ClearInterruptFlags();
#if defined(_MPU_STM32MP13)
		// aka HAL_DMA_Init: force request 0 for MEM2MEM
		if (!from_periph && !to_periph) {
			byte mux_idx;
			stduint mux_base = _DMAMUX_StreamSel(sel.getID(), st, mux_idx);
			Reference(mux_base + mux_idx * 4) = 0;// DMA_REQUEST_MEM2MEM
			sel.streamRequests[st] = 0;
		}
#endif
		// aka HAL_DMA_Init tail: set ready state + clear error code
		sel.streamStates[st] = _DMA_STATE_READY;
		sel.streamErrors[st] = _DMA_ERROR_NONE;
		return true;
	}

	void DMAChannel::setFIFOThreshold(DMAChannel::FIFOThreshold hold) const {
		using namespace DMAReg;
		byte st = getParent().getID();
		DMA_t& sel = getParent().getParent();
		Reference fcr = sel[FCR[st]];
		fcr.maset(_DMA_SxFCR_POS_FTH, 2, _IMM(hold));
	}

#if defined(_MPU_STM32MP13)
	// aka HAL DMA_CheckFifoParam: verify FIFO threshold vs memory data size vs memory burst.
	// memory_size: 0=byte, 1=half-word, 2=word ; threshold: 0=1/4,1=1/2,2=3/4,3=full ; burst: 0..3
	static bool _DMA_CheckFifoParam(byte memory_size, byte threshold, byte burst) {
		if (memory_size == 0) { // byte
			switch (threshold) {
			case 0: case 2: // 1/4 or 3/4
				if ((burst & 2) == 2) return false; // INC8 / INC16
				break;
			case 1: // 1/2
				if (burst == 3) return false; // INC16
				break;
			case 3: break; // full: always ok
			}
		} else if (memory_size == 1) { // half-word
			switch (threshold) {
			case 0: case 2: return false; // 1/4 or 3/4: always error
			case 1:
				if ((burst & 2) == 2) return false;
				break;
			case 3:
				if (burst == 3) return false;
				break;
			}
		} else { // word
			switch (threshold) {
			case 0: case 1: case 2: return false; // 1/4, 1/2, 3/4: always error
			case 3:
				if ((burst & 2) == 2) return false;
				break;
			}
		}
		return true;
	}

	bool DMAChannel::setFIFO(bool ena, Burst mem_burst, Burst periph_burst, FIFOThreshold threshold) const {
		using namespace DMAReg;
		byte st = getParent().getID();
		DMA_t& sel = getParent().getParent();
		Reference cr = sel[CR[st]];
		Reference fcr = sel[FCR[st]];
		// DMDIS=1 disables direct mode, i.e. enables FIFO mode (aka DMA_FIFOMODE_ENABLE)
		fcr.setof(_DMA_SxFCR_POS_DMDIS, ena);
		if (ena) {
			// memory/peripheral burst + FIFO threshold are only written when FIFO is enabled
			cr.maset(_DMA_SxCR_POS_MBURST, 2, _IMM(mem_burst));
			cr.maset(_DMA_SxCR_POS_PBURST, 2, _IMM(periph_burst));
			setFIFOThreshold(threshold);
			// compatibility check only for non-single memory burst (aka DMA_CheckFifoParam)
			if (mem_burst != Burst::Single) {
				byte ms = (byte)cr.masof(_DMA_SxCR_POS_MSIZE, 2);
				if (!_DMA_CheckFifoParam(ms, (byte)threshold, (byte)mem_burst)) {
					sel.streamErrors[st] = _DMA_ERROR_PARAM;
					return false;
				}
			}
		}
		return true;
	}
#endif

	// cancel working modes, aka HAL_DMA_DeInit
	void DMAStream::canMode(void) const {
		using namespace DMAReg;
		DMA_t& sel = getParent();
		byte st = getID();
		sel.enClock();
		sel[CR[st]] = 0;
		sel[NDTR[st]] = 0;
		sel[PAR[st]] = 0;
		sel[M0AR[st]] = 0;
		sel[M1AR[st]] = 0;
		sel[FCR[st]] = 0x00000021;
		ClearInterruptFlags();
		while (sel[CR[st]].bitof(_DMA_SxCR_POS_EN));
#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
		// reset DMAMUX channel (aka HAL_DMA_DeInit DMAMUX part)
		byte mux_index;
		stduint mux_base = _DMAMUX_StreamSel(sel.getID(), st, mux_index);
		Reference(mux_base + mux_index * 4) = 0;// CCR = 0
		Reference(mux_base + 0x084U).setof(mux_index, true);// clear CFR
#if defined(_MPU_STM32MP13)
		byte rg_index;
		if (_DMAMUX_UseReqGen(sel, st, rg_index)) {
			Reference(mux_base + 0x100U + rg_index * 4) = 0;// RGCR = 0
			Reference(mux_base + 0x144U).setof(rg_index, true);// clear RGCFR
		}
#endif
#endif
		// clear callbacks (aka HAL_DMA_DeInit)
		sel.XferCpltCallback = 0;
		sel.XferHalfCallback = 0;
		sel.XferM1CpltCallback = 0;
		sel.XferM1HalfCpltCallback = 0;
		sel.XferErrorCallback = 0;
		sel.XferAbortCallback = 0;
		// reset state & error code (aka HAL_DMA_DeInit)
		sel.streamErrors[st] = _DMA_ERROR_NONE;
		sel.streamStates[st] = _DMA_STATE_RESET;
	}

	void DMAStream::setRequest(stduint request_id) const {
		using namespace DMAReg;
		DMA_t& sel = getParent();
		byte st = getID();
#if defined(_MCU_STM32F4x)
		sel[CR[st]].maset(_DMA_SxCR_POS_CHSEL, 3, request_id);
#elif defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
		// DMA1 Stream N -> DMAMUX1 Channel N ; DMA2 Stream N -> DMAMUX1 Channel N+8
		// DMA3 Stream N -> DMAMUX2 Channel N (MP13 only)
		byte mux_index;
		stduint mux_base = _DMAMUX_StreamSel(sel.getID(), st, mux_index);
		Reference(mux_base + mux_index * 4) = request_id;
#if defined(_MPU_STM32MP13)
		sel.streamRequests[st] = (byte)(request_id & 0xFF);// remember for request-generator routing
		_DMAMUX_ClearOverrun(sel, st);// clear overrun flags (aka HAL_DMA_Init DMAMUX part)
		byte rg_index;
		if (_DMAMUX_UseReqGen(sel, st, rg_index))
			Reference(mux_base + 0x100U + rg_index * 4) = 0;// reset RGCR (aka HAL_DMA_Init)
#endif
#else
		(void)st; (void)request_id;
#endif
	}

	void DMAStream::setInterruptPriority(byte preempt, byte sub_priority) const {
		DMA_t& sel = getParent();
#if defined(_MPU_STM32MP13)
		(void)sub_priority;
		GIC.setPriority(DMAx_Stream_Requests_list[sel.getID()][getID()], preempt);
#else
		NVIC.setPriority(DMAx_Stream_Requests_list[sel.getID()][getID()], preempt, sub_priority);
#endif
	}

	void DMAStream::enInterruptNVIC(bool ena) const {
		DMA_t& sel = getParent();
#if defined(_MPU_STM32MP13)
		GIC.enInterrupt(DMAx_Stream_Requests_list[sel.getID()][getID()], ena);
#else
		NVIC.setAble(DMAx_Stream_Requests_list[sel.getID()][getID()], ena);
#endif
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
			ClearInterruptFlags();
#if defined(_MPU_STM32MP13)
			_DMAMUX_ClearOverrun(sel, st);
			_DMAMUX_EnableOverrunIT(sel, st);
#endif
			setInterruptSub(true);
#if defined(_MPU_STM32MP13)
			sel[FCR[st]].setof(_DMA_SxFCR_POS_FEIE, false);// MP13 Start_IT does not enable FEIE
#endif
			enInterruptNVIC(true);
			sel.streamStates[st] = _DMA_STATE_BUSY;
			sel.streamErrors[st] = _DMA_ERROR_NONE;
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
		ClearInterruptFlags();
#if defined(_MPU_STM32MP13)
		_DMAMUX_ClearOverrun(sel, st);
#endif
		sel.streamStates[st] = _DMA_STATE_BUSY;
		sel.streamErrors[st] = _DMA_ERROR_NONE;
		enAble(true);

		uint64 tickstart = SysTick::getTick();
		byte posi = flagBase();
		DMARegType isr_reg = (st & 0b100) ? HISR : LISR;
		while (!(sel[isr_reg] & (1U << (posi + _DMA_SxFLAG_POS_TCIF)))) {
			if (sel[isr_reg] & (1U << (posi + _DMA_SxFLAG_POS_FEIF))) {
				sel.streamErrors[st] |= _DMA_ERROR_FE;
				sel[refIFCR()].setof(posi + _DMA_SxFLAG_POS_FEIF, true);
			}
			if (sel[isr_reg] & (1U << (posi + _DMA_SxFLAG_POS_DMEIF))) {
				sel.streamErrors[st] |= _DMA_ERROR_DME;
				sel[refIFCR()].setof(posi + _DMA_SxFLAG_POS_DMEIF, true);
			}
			if (sel[isr_reg] & (1U << (posi + _DMA_SxFLAG_POS_TEIF))) {
				sel.streamErrors[st] |= _DMA_ERROR_TE;
				ClearInterruptFlags();
				sel.streamStates[st] = _DMA_STATE_READY;
				return false;
			}
			if ((SysTick::getTick() - tickstart) > DMA_TIMEOUT_VALUE) {
				sel.streamErrors[st] = _DMA_ERROR_TIMEOUT;
				Abort();
				return false;
			}
		}
#if defined(_MPU_STM32MP13)
		// DMAMUX sync / request-generator overrun (aka HAL_DMA_PollForTransfer tail)
		byte mux_idx;
		stduint mux_base = _DMAMUX_StreamSel(sel.getID(), st, mux_idx);
		byte rg_index;
		if (_DMAMUX_UseReqGen(sel, st, rg_index) && Reference(mux_base + 0x140U).bitof(rg_index)) {
			Reference(mux_base + 0x144U).setof(rg_index, true);// clear RGCFR
			sel.streamErrors[st] |= _DMA_ERROR_REQGEN;
		}
		if (Reference(mux_base + 0x080U).bitof(mux_idx)) {
			Reference(mux_base + 0x084U).setof(mux_idx, true);// clear CFR
			sel.streamErrors[st] |= _DMA_ERROR_SYNC;
		}
#endif
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
#if defined(_MPU_STM32MP13)
		_DMAMUX_DisableOverrunIT(sel, st);
#endif
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
#if defined(_MPU_STM32MP13)
		_DMAMUX_ClearOverrun(sel, st);
#endif
		if (method == IOMethod::Rupt) {
			// Enable common interrupts (TCIE | TEIE | DMEIE)
			setInterruptSub(true);
			// Double buffer: also enable HT if half callbacks are registered
			if (sel.XferHalfCallback || sel.XferM1HalfCpltCallback)
				sel[CR[st]].setof(_DMA_SxCR_POS_HTIE, true);
#if defined(_MPU_STM32MP13)
			_DMAMUX_EnableOverrunIT(sel, st);
#endif
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

#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	// AKA HAL_DMAEx_ConfigMuxSync: configure DMAMUX synchronization for this stream
	// Clear SE+EGE before applying new config; keeps existing DMAREQ_ID.
	// DMAMUX_CxCR [7:0] DMAREQ_ID, [8] SOIE, [9] EGE, [16] SE, [18:17] SPOL, [24:19] NBREQ, [29:25] SYNC_ID
	bool DMAStream::setMuxSync(stduint signal_id, MuxPolarity pol, bool ena, bool event_ena, stduint request_num) const {
		DMA_t& sel = getParent();
		byte st = getID();
		byte mux_idx;
		stduint ccr_addr = _DMAMUX_StreamSel(sel.getID(), st, mux_idx) + mux_idx * 4;
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
#if defined(_MPU_STM32MP13)
		byte rg_index;
		if (!_DMAMUX_UseReqGen(sel, st, rg_index)) {
			sel.streamErrors[st] = _DMA_ERROR_NOT_SUPPORTED;
			return false;
		}
		stduint rgcr_addr = _DMAMUX_Base(sel.getID()) + 0x100U + rg_index * 0x04U;
#else
		stduint rgcr_addr = _DMAMUX_Base(sel.getID()) + 0x100U + st * 0x04U;
#endif
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
#if defined(_MPU_STM32MP13)
		byte rg_index;
		if (!_DMAMUX_UseReqGen(sel, st, rg_index)) {
			sel.streamErrors[st] = _DMA_ERROR_NOT_SUPPORTED;
			return false;
		}
		stduint rgcr_addr = _DMAMUX_Base(sel.getID()) + 0x100U + rg_index * 0x04U;
#else
		stduint rgcr_addr = _DMAMUX_Base(sel.getID()) + 0x100U + st * 0x04U;
#endif
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
		byte mux_idx;
		stduint mux_base = _DMAMUX_StreamSel(sel.getID(), st, mux_idx);

		// Check DMAMUX synchronization overrun for this stream's channel
		Reference csr = Reference(mux_base + 0x080U);
		if (csr.bitof(mux_idx)) {
			// Disable sync overrun interrupt
			stduint ccr_addr = mux_base + mux_idx * 4;
			Reference(ccr_addr) &= ~_DMAMUX_CxCR_SOIE;
			// Clear sync overrun flag (write 1 to CFR bit)
			Reference(mux_base + 0x084U).setof(mux_idx, true);
			sel.streamErrors[st] |= _DMA_ERROR_SYNC;
			asserv(sel.XferErrorCallback)();
		}

		// Check request generator overrun (generator index = request id - 1)
#if defined(_MPU_STM32MP13)
		byte rg_index;
		if (_DMAMUX_UseReqGen(sel, st, rg_index)) {
			Reference rgsr = Reference(mux_base + 0x140U);
			if (rgsr.bitof(rg_index)) {
				// Disable request generator overrun interrupt
				Reference rgcr = Reference(mux_base + 0x100U + rg_index * 0x04U);
				rgcr &= ~_DMAMUX_RGxCR_OIE;
				// Clear request generator overrun flag (write 1 to RGCFR bit)
				Reference(mux_base + 0x144U).setof(rg_index, true);
				sel.streamErrors[st] |= _DMA_ERROR_REQGEN;
				asserv(sel.XferErrorCallback)();
			}
		}
#else
		Reference rgsr = Reference(mux_base + 0x140U);
		if (rgsr.bitof(st)) {
			// Disable request generator overrun interrupt
			Reference rgcr = Reference(mux_base + 0x100U + st * 0x04U);
			rgcr &= ~_DMAMUX_RGxCR_OIE;
			// Clear request generator overrun flag (write 1 to RGCFR bit)
			Reference(mux_base + 0x144U).setof(st, true);
			sel.streamErrors[st] |= _DMA_ERROR_REQGEN;
			asserv(sel.XferErrorCallback)();
		}
#endif
	}
#endif

#endif

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

	Reference DMA_t::operator[](DMAReg::DMARegType idx) {
		return Reference(_REFADDR_DMA[DMA_ID] + _IMMx4(idx));
	}

	bool DMA_t::enClock(bool ena) {
#if defined(_MPU_STM32MP13)
		using namespace RCCReg;
		// AHB2 enable bits: DMA1=0, DMA2=1, DMAMUX1=2, DMA3=3, DMAMUX2=4
		// DMA1/DMA2 are routed by DMAMUX1, DMA3 by DMAMUX2; clock the router together.
		stduint dma_bit = 0, mux_bit = 0;
		if (DMA_ID == 1)      { dma_bit = 0; mux_bit = 2; }
		else if (DMA_ID == 2) { dma_bit = 1; mux_bit = 2; }
		else                  { dma_bit = 3; mux_bit = 4; }
		RCCReg::RCCReg reg = ena ? MP_AHB2ENSETR : MP_AHB2ENCLRR;
		RCC[reg] = _IMM1S(dma_bit) | _IMM1S(mux_bit);
		if (ena != (RCC[MP_AHB2ENSETR].bitof(dma_bit) && RCC[MP_AHB2ENSETR].bitof(mux_bit)))
			return false;
		return true;
#else
		Reference(RCC_DMAx_addrs[DMA_ID - 1]).setof(RCC_DMAx_bitpos[DMA_ID - 1], ena);
	#if defined(_MCU_STM32H7x)
		// DMAMUX1 is the request router for both DMA1 and DMA2; clock it together
		Reference(_RCC_AHB1ENR_ADDR).setof(_RCC_AHB1ENR_POSI_ENCLK_DMAMUX1, ena);
	#endif
		if (ena != Reference(RCC_DMAx_addrs[DMA_ID - 1]).bitof(RCC_DMAx_bitpos[DMA_ID - 1]))
			return false;
		return true;
#endif
	}


	DMA_t DMAr(0), DMA1(1), DMA2(2);
#if defined(_MPU_STM32MP13)
	DMA_t DMA3(3);
#endif

#endif
}
