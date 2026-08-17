// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Master Direct Memory Access, MDMA
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

#include "../../../inc/cpp/Device/MDMA"
#include "../../../inc/cpp/Device/RCC/RCCAddress"
#include "../../../inc/cpp/Device/NVIC"
#include "../../../inc/cpp/Device/SysTick"
#include "../../../inc/cpp/Device/Interrupt/interrupt_tab.h"

namespace uni {
#if defined(_MCU_STM32H7x)

	// AKA MDMA_BASE (D1 AHB1 peripheral domain)
	static const stduint _MDMA_ADDR = D1_AHB1PERIPH_BASE + 0x0000;
	// AKA HAL_TIMEOUT_MDMA_ABORT (5 ms; SysTick::getTick() tick == ms)
	#define _MDMA_ABORT_TIMEOUT 5U
	// transfer polling timeout (same scale as DMA_TIMEOUT_VALUE)
	#define _MDMA_TIMEOUT_VALUE 0x1FFFFFFUL

	MDMA_t MDMA;

	Reference MDMA_t::operator[](MDMAReg::Global idx) const {
		return Reference(_MDMA_ADDR + _IMMx4(idx));
	}

	bool MDMA_t::enClock(bool ena) const {
		Reference(_RCC_AHB3ENR_ADDR).setof(_RCC_AHB3ENR_POSI_ENCLK_MDMA, ena);
		return Reference(_RCC_AHB3ENR_ADDR).bitof(_RCC_AHB3ENR_POSI_ENCLK_MDMA) == ena;
	}

	Reference MDMAChannel::operator[](MDMAReg::Chan idx) const {
		return Reference(_MDMA_ADDR + 0x40 + getID() * 0x40 + _IMMx4(idx));
	}

	void MDMAChannel::enAble(bool ena) const {
		self[MDMAReg::Chan::CCR].setof(_MDMA_CCR_POS_EN, ena);
	}

	// map MDMAInc -> SINC(2b) / SINCOS(2b) field values (aka MDMA_SRC_INC_* / MDMA_SRC_DEC_*)
	static void _MDMA_mapInc(MDMAInc inc, stduint& sinc, stduint& sincos) {
		switch (inc) {
		case MDMAInc::Fixed:         sinc = 0; sincos = 0; break;
		case MDMAInc::IncByte:       sinc = 2; sincos = 0; break;
		case MDMAInc::IncHalfword:   sinc = 2; sincos = 1; break;
		case MDMAInc::IncWord:       sinc = 2; sincos = 2; break;
		case MDMAInc::IncDoubleword: sinc = 2; sincos = 3; break;
		case MDMAInc::DecByte:       sinc = 3; sincos = 0; break;
		case MDMAInc::DecHalfword:   sinc = 3; sincos = 1; break;
		case MDMAInc::DecWord:       sinc = 3; sincos = 2; break;
		default:                     sinc = 3; sincos = 3; break; // DecDoubleword
		}
	}

	bool MDMAChannel::setMode(stduint request, MDMATrigger trigger,
		MDMAInc srcInc, MDMAInc dstInc, MDMASize srcSize, MDMASize dstSize,
		MDMAAlign align, MDMABurst srcBurst, MDMABurst dstBurst, stduint bufferLen) const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		// AKA IS_MDMA_BUFFER_TRANSFER_LENGTH: 1 .. 0xFE (BufferTransferLength - 1 fits TLEN 8b)
		if (!bufferLen || bufferLen > 0xFE) return false;
		// AKA HAL_MDMA_Init: disable + wait EN=0 (timeout -> TIMEOUT error, state ERROR)
		enAble(false);
		uint64 tickstart = SysTick::getTick();
		while (self[Chan::CCR].bitof(_MDMA_CCR_POS_EN)) {
			if ((SysTick::getTick() - tickstart) > _MDMA_ABORT_TIMEOUT) {
				p.channelErrors[id] = _MDMA_ERROR_TIMEOUT;
				p.channelStates[id] = _MDMA_STATE_ERROR;
				return false;
			}
		}
		stduint sinc, sincos, dinc, dincos;
		_MDMA_mapInc(srcInc, sinc, sincos);
		_MDMA_mapInc(dstInc, dinc, dincos);
		stduint ctcr = 0;
		ctcr |= (sinc << _MDMA_CTCR_POS_SINC) | (dinc << _MDMA_CTCR_POS_DINC);
		ctcr |= (sincos << _MDMA_CTCR_POS_SINCOS) | (dincos << _MDMA_CTCR_POS_DINCOS);
		ctcr |= ((stduint)srcSize << _MDMA_CTCR_POS_SSIZE) | ((stduint)dstSize << _MDMA_CTCR_POS_DSIZE);
		if (align == MDMAAlign::PackEnable) ctcr |= (1U << _MDMA_CTCR_POS_PKE);
		else ctcr |= ((stduint)align << _MDMA_CTCR_POS_PAM);
		ctcr |= ((stduint)srcBurst << _MDMA_CTCR_POS_SBURST) | ((stduint)dstBurst << _MDMA_CTCR_POS_DBURST);
		ctcr |= ((bufferLen - 1) << _MDMA_CTCR_POS_TLEN);
		ctcr |= ((stduint)trigger << _MDMA_CTCR_POS_TRGM);
		// AKA MDMA_Init: SW request sets SWRM|BWM and resets CTBR; HW request sets CTBR.TSEL
		if (request == MDMA_REQUEST_SW) {
			ctcr |= (1U << _MDMA_CTCR_POS_SWRM) | (1U << _MDMA_CTCR_POS_BWM);
			self[Chan::CTBR] = 0;
		} else {
			self[Chan::CTBR] = request & 0x3FU; // AKA MDMA_CTBR_TSEL (6b)
		}
		self[Chan::CTCR] = ctcr;
		self[Chan::CBNDTR] = 0;
		self[Chan::CBRUR] = 0;
		self[Chan::CLAR] = 0;
		// AKA HAL_MDMA_Init tail: reset linked-list bookkeeping, clear error, set READY
		p.firstNode[id] = 0;
		p.lastNode[id] = 0;
		p.nodeCounter[id] = 0;
		p.channelErrors[id] = _MDMA_ERROR_NONE;
		p.channelStates[id] = _MDMA_STATE_READY;
		return true;
	}

	void MDMAChannel::setBlockOffset(int32 srcOffset, int32 dstOffset) const {
		using namespace MDMAReg;
		Reference cbndtr = self[Chan::CBNDTR];
		stduint cbrur = 0;
		if (srcOffset < 0) {
			cbndtr.setof(_MDMA_CBNDTR_POS_BRSUM);
			cbrur |= ((stduint)(-srcOffset) & 0xFFFFU);
		} else {
			cbndtr.rstof(_MDMA_CBNDTR_POS_BRSUM);
			cbrur |= ((stduint)srcOffset & 0xFFFFU);
		}
		if (dstOffset < 0) {
			cbndtr.setof(_MDMA_CBNDTR_POS_BRDUM);
			cbrur |= (((stduint)(-dstOffset) & 0xFFFFU) << _MDMA_CBRUR_POS_DUV);
		} else {
			cbndtr.rstof(_MDMA_CBNDTR_POS_BRDUM);
			cbrur |= (((stduint)dstOffset & 0xFFFFU) << _MDMA_CBRUR_POS_DUV);
		}
		self[Chan::CBRUR] = cbrur;
	}

	void MDMAChannel::setPriority(MDMAPriority priority) const {
		self[MDMAReg::Chan::CCR].maset(_MDMA_CCR_POS_PL, 2, (stduint)priority);
	}

	void MDMAChannel::setEndian(MDMAEndian endian) const {
		using namespace MDMAReg;
		Reference ccr = self[Chan::CCR];
		ccr &= ~((1U << _MDMA_CCR_POS_BEX) | (1U << _MDMA_CCR_POS_HEX) | (1U << _MDMA_CCR_POS_WEX));
		if (endian != MDMAEndian::Preserve)
			ccr.setof(_MDMA_CCR_POS_BEX + (stduint)endian - 1);
	}

	bool MDMAChannel::setPostRequestMask(pureptr_t maskAddr, uint32 maskData) const {
		using namespace MDMAReg;
		Reference ctcr = self[Chan::CTCR];
		// AKA HAL_MDMA_ConfigPostRequestMask: only for HW request (SWRM == 0)
		if (ctcr.bitof(_MDMA_CTCR_POS_SWRM)) return false;
		self[Chan::CMAR] = _IMM(maskAddr);
		self[Chan::CMDR] = maskData;
		if (maskAddr == 0) ctcr.rstof(_MDMA_CTCR_POS_BWM);
		else ctcr.setof(_MDMA_CTCR_POS_BWM);
		return true;
	}

	// AKA HAL_MDMA_DeInit
	void MDMAChannel::canMode() const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		enAble(false);
		self[Chan::CCR] = 0;
		self[Chan::CTCR] = 0;
		self[Chan::CBNDTR] = 0;
		self[Chan::CSAR] = 0;
		self[Chan::CDAR] = 0;
		self[Chan::CBRUR] = 0;
		self[Chan::CLAR] = 0;
		self[Chan::CTBR] = 0;
		self[Chan::CMAR] = 0;
		self[Chan::CMDR] = 0;
		// clear all flags (TE|CTC|BRT|BT|BFTC)
		self[Chan::CIFCR] = 0x1F;
		// clear callbacks (aka HAL_MDMA_DeInit)
		p.XferCpltCallback[id] = 0;
		p.XferBufferCpltCallback[id] = 0;
		p.XferBlockCpltCallback[id] = 0;
		p.XferRepeatBlockCpltCallback[id] = 0;
		p.XferErrorCallback[id] = 0;
		p.XferAbortCallback[id] = 0;
		p.firstNode[id] = 0;
		p.lastNode[id] = 0;
		p.nodeCounter[id] = 0;
		p.channelErrors[id] = _MDMA_ERROR_NONE;
		p.channelStates[id] = _MDMA_STATE_RESET;
	}

	// AKA MDMA_SetConfig (static in HAL)
	void MDMAChannel::setConfig(pureptr_t srcAddr, pureptr_t dstAddr, stduint blockDataLength, stduint blockCount) const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		Reference cbndtr = self[Chan::CBNDTR];
		// AKA MODIFY_REG(CBNDTR, BNDT, BlockDataLength) ; MODIFY_REG(CBNDTR, BRC, (BlockCount-1)<<BRC)
		cbndtr.maset(_MDMA_CBNDTR_POS_BNDT, 17, blockDataLength);
		cbndtr.maset(_MDMA_CBNDTR_POS_BRC, 12, blockCount - 1);
		// clear all interrupt flags (TE|CTC|BRT|BT|BFTC)
		self[Chan::CIFCR] = 0x1F;
		self[Chan::CDAR] = _IMM(dstAddr);
		self[Chan::CSAR] = _IMM(srcAddr);
		// AKA MDMA_SetConfig bus selection (0x2000xxxx or 0x00xxxxxx -> AHB bus)
		stduint srcMask = _IMM(srcAddr) & 0xFF000000U;
		if ((srcMask == 0x20000000U) || (srcMask == 0x00000000U)) self[Chan::CTBR].setof(_MDMA_CTBR_POS_SBUS);
		else self[Chan::CTBR].rstof(_MDMA_CTBR_POS_SBUS);
		stduint dstMask = _IMM(dstAddr) & 0xFF000000U;
		if ((dstMask == 0x20000000U) || (dstMask == 0x00000000U)) self[Chan::CTBR].setof(_MDMA_CTBR_POS_DBUS);
		else self[Chan::CTBR].rstof(_MDMA_CTBR_POS_DBUS);
		// AKA MDMA_SetConfig: link to first node of the list (0 when no linked list)
		self[Chan::CLAR] = _IMM(p.firstNode[id]);
	}

	// AKA HAL_MDMA_Start / HAL_MDMA_Start_IT / HAL_MDMA_PollForTransfer
	bool MDMAChannel::Transfer(pureptr_t srcAddr, pureptr_t dstAddr, stduint blockDataLength, stduint blockCount, IOMethod method) const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		// AKA IS_MDMA_TRANSFER_LENGTH / IS_MDMA_BLOCK_COUNT
		if (!blockDataLength || blockDataLength > 65536) return false;
		if (!blockCount || blockCount > 4096) return false;
		// AKA HAL_MDMA_Start: only from READY (else HAL_BUSY)
		if (p.channelStates[id] != _MDMA_STATE_READY) return false;

		if (method == IOMethod::Rupt) {
			// AKA HAL_MDMA_Start_IT
			enAble(false);
			setConfig(srcAddr, dstAddr, blockDataLength, blockCount);
			setInterruptSub(true);
			NVIC.setAble(IRQ_MDMA, true);
			p.channelErrors[id] = _MDMA_ERROR_NONE;
			p.channelStates[id] = _MDMA_STATE_BUSY;
			enAble(true);
			if (self[Chan::CTCR].bitof(_MDMA_CTCR_POS_SWRM))
				self[Chan::CCR].setof(_MDMA_CCR_POS_SWRQ); // AKA activate SW request
			return true;
		}

		if (method != IOMethod::Loop) return false; // MDMA sub-mode N/A for MDMA itself

		// IOMethod::Loop: AKA HAL_MDMA_Start + HAL_MDMA_PollForTransfer(FULL)
		enAble(false);
		setConfig(srcAddr, dstAddr, blockDataLength, blockCount);
		p.channelErrors[id] = _MDMA_ERROR_NONE;
		p.channelStates[id] = _MDMA_STATE_BUSY;
		enAble(true);
		if (self[Chan::CTCR].bitof(_MDMA_CTCR_POS_SWRM))
			self[Chan::CCR].setof(_MDMA_CCR_POS_SWRQ); // AKA activate SW request

		uint64 tickstart = SysTick::getTick();
		Reference cisr = self[Chan::CISR];
		while (!cisr.bitof(_MDMA_CISR_POS_CTCIF)) {
			// AKA HAL_MDMA_PollForTransfer: transfer error -> record error -> abort
			if (cisr.bitof(_MDMA_CISR_POS_TEIF)) {
				readErrorSource();
				Abort();
				return false;
			}
			if ((SysTick::getTick() - tickstart) > _MDMA_TIMEOUT_VALUE) {
				p.channelErrors[id] |= _MDMA_ERROR_TIMEOUT;
				Abort();
				return false;
			}
		}
		// AKA HAL_MDMA_PollForTransfer(FULL_TRANSFER) tail: clear flags + state READY
		self[Chan::CIFCR] = 0x1F;
		p.channelStates[id] = _MDMA_STATE_READY;
		return true;
	}

	// AKA HAL_MDMA_Abort (blocking)
	bool MDMAChannel::Abort() const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		if (p.channelStates[id] != _MDMA_STATE_BUSY) {
			p.channelErrors[id] = _MDMA_ERROR_NO_XFER;
			return false;
		}
		// AKA disable all transfer interrupts
		Reference ccr = self[Chan::CCR];
		ccr.rstof(_MDMA_CCR_POS_TEIE);
		ccr.rstof(_MDMA_CCR_POS_CTCIE);
		ccr.rstof(_MDMA_CCR_POS_BRTIE);
		ccr.rstof(_MDMA_CCR_POS_BTIE);
		ccr.rstof(_MDMA_CCR_POS_TCIE);
		enAble(false);
		// AKA wait EN=0 with timeout
		uint64 tickstart = SysTick::getTick();
		while (self[Chan::CCR].bitof(_MDMA_CCR_POS_EN)) {
			if ((SysTick::getTick() - tickstart) > _MDMA_ABORT_TIMEOUT) {
				p.channelErrors[id] |= _MDMA_ERROR_TIMEOUT;
				p.channelStates[id] = _MDMA_STATE_ERROR;
				return false;
			}
		}
		self[Chan::CIFCR] = 0x1F; // clear all flags
		p.channelStates[id] = _MDMA_STATE_READY;
		return true;
	}

	// AKA HAL_MDMA_Abort_IT (non-blocking)
	bool MDMAChannel::AbortRupt() const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		if (p.channelStates[id] != _MDMA_STATE_BUSY) {
			p.channelErrors[id] = _MDMA_ERROR_NO_XFER;
			return false;
		}
		p.channelStates[id] = _MDMA_STATE_ABORT;
		enAble(false);
		return true;
	}

	// AKA HAL_MDMA_GenerateSWRequest
	bool MDMAChannel::GenerateSWRequest() const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		Reference ccr = self[Chan::CCR];
		if (!ccr.bitof(_MDMA_CCR_POS_EN)) {
			p.channelErrors[id] = _MDMA_ERROR_NO_XFER;
			return false;
		}
		if (self[Chan::CISR].bitof(_MDMA_CISR_POS_CRQA) || !self[Chan::CTCR].bitof(_MDMA_CTCR_POS_SWRM)) {
			p.channelErrors[id] = _MDMA_ERROR_BUSY;
			return false;
		}
		ccr.setof(_MDMA_CCR_POS_SWRQ);
		return true;
	}

	// AKA HAL_MDMA_IRQHandler
	void MDMAChannel::HandleIRQ() const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		// AKA GISR0 general interrupt flag check for this channel
		if (!Reference(_MDMA_ADDR).bitof(id)) return;
		Reference ccr = self[Chan::CCR];
		Reference cisr = self[Chan::CISR];
		Reference cifcr = self[Chan::CIFCR];
		// transfer error
		if (cisr.bitof(_MDMA_CISR_POS_TEIF)) {
			if (ccr.bitof(_MDMA_CCR_POS_TEIE)) {
				ccr.rstof(_MDMA_CCR_POS_TEIE); // AKA DISABLE_IT(TE)
				readErrorSource();
				cifcr.setof(_MDMA_CISR_POS_TEIF);
			}
		}
		// buffer transfer complete (CISR.TCIF / BFTC)
		if (cisr.bitof(_MDMA_CISR_POS_TCIF)) {
			if (ccr.bitof(_MDMA_CCR_POS_TCIE)) {
				cifcr.setof(_MDMA_CISR_POS_TCIF);
				if (p.XferBufferCpltCallback[id]) p.XferBufferCpltCallback[id]();
			}
		}
		// block transfer complete
		if (cisr.bitof(_MDMA_CISR_POS_BTIF)) {
			if (ccr.bitof(_MDMA_CCR_POS_BTIE)) {
				cifcr.setof(_MDMA_CISR_POS_BTIF);
				if (p.XferBlockCpltCallback[id]) p.XferBlockCpltCallback[id]();
			}
		}
		// repeated block transfer complete
		if (cisr.bitof(_MDMA_CISR_POS_BRTIF)) {
			if (ccr.bitof(_MDMA_CCR_POS_BRTIE)) {
				cifcr.setof(_MDMA_CISR_POS_BRTIF);
				if (p.XferRepeatBlockCpltCallback[id]) p.XferRepeatBlockCpltCallback[id]();
			}
		}
		// channel transfer complete
		if (cisr.bitof(_MDMA_CISR_POS_CTCIF)) {
			if (ccr.bitof(_MDMA_CCR_POS_CTCIE)) {
				// AKA disable all transfer interrupts
				ccr.rstof(_MDMA_CCR_POS_TEIE);
				ccr.rstof(_MDMA_CCR_POS_CTCIE);
				ccr.rstof(_MDMA_CCR_POS_BRTIE);
				ccr.rstof(_MDMA_CCR_POS_BTIE);
				ccr.rstof(_MDMA_CCR_POS_TCIE);
				if (p.channelStates[id] == _MDMA_STATE_ABORT) {
					p.channelStates[id] = _MDMA_STATE_READY;
					if (p.XferAbortCallback[id]) p.XferAbortCallback[id]();
					return;
				}
				cifcr.setof(_MDMA_CISR_POS_CTCIF);
				p.channelStates[id] = _MDMA_STATE_READY;
				if (p.XferCpltCallback[id]) p.XferCpltCallback[id]();
			}
		}
		// AKA error management tail
		if (p.channelErrors[id] != _MDMA_ERROR_NONE) {
			p.channelStates[id] = _MDMA_STATE_ABORT;
			enAble(false);
			uint64 tickstart = SysTick::getTick();
			while (self[Chan::CCR].bitof(_MDMA_CCR_POS_EN)) {
				if ((SysTick::getTick() - tickstart) > _MDMA_ABORT_TIMEOUT) break;
			}
			if (self[Chan::CCR].bitof(_MDMA_CCR_POS_EN))
				p.channelStates[id] = _MDMA_STATE_ERROR;
			else
				p.channelStates[id] = _MDMA_STATE_READY;
			if (p.XferErrorCallback[id]) p.XferErrorCallback[id]();
		}
	}

	// AKA HAL_MDMA error source decode (CESR -> ErrorCode), shared by poll + IRQ paths
	void MDMAChannel::readErrorSource() const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		stduint errorFlag = self[Chan::CESR];
		if (!(errorFlag & (1U << _MDMA_CESR_POS_TED)))
			p.channelErrors[id] |= _MDMA_ERROR_READ_XFER;
		else
			p.channelErrors[id] |= _MDMA_ERROR_WRITE_XFER;
		if (errorFlag & (1U << _MDMA_CESR_POS_TEMD)) p.channelErrors[id] |= _MDMA_ERROR_MASK_DATA;
		if (errorFlag & (1U << _MDMA_CESR_POS_TELD)) p.channelErrors[id] |= _MDMA_ERROR_LINKED_LIST;
		if (errorFlag & (1U << _MDMA_CESR_POS_ASE))  p.channelErrors[id] |= _MDMA_ERROR_ALIGNMENT;
		if (errorFlag & (1U << _MDMA_CESR_POS_BSE))  p.channelErrors[id] |= _MDMA_ERROR_BLOCK_SIZE;
	}

	// AKA HAL_MDMA_Start_IT interrupt selection: TE|CTC always, BT/BRT/BFTC per registered callback
	void MDMAChannel::setInterruptSub(bool ena_total) const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		Reference ccr = self[Chan::CCR];
		ccr.setof(_MDMA_CCR_POS_TEIE, ena_total);
		ccr.setof(_MDMA_CCR_POS_CTCIE, ena_total);
		ccr.setof(_MDMA_CCR_POS_BTIE, ena_total && p.XferBlockCpltCallback[id]);
		ccr.setof(_MDMA_CCR_POS_BRTIE, ena_total && p.XferRepeatBlockCpltCallback[id]);
		ccr.setof(_MDMA_CCR_POS_TCIE, ena_total && p.XferBufferCpltCallback[id]);
	}

	// AKA HAL_MDMA_LinkedList_CreateNode
	bool MDMAChannel::CreateNode(MDMA_LinkNode& node, const MDMA_LinkNodeConf& conf) const {
		// AKA IS_MDMA_TRANSFER_LENGTH / IS_MDMA_BLOCK_COUNT / IS_MDMA_BUFFER_TRANSFER_LENGTH
		if (!conf.blockDataLength || conf.blockDataLength > 65536) return false;
		if (!conf.blockCount || conf.blockCount > 4096) return false;
		if (!conf.bufferLen || conf.bufferLen > 0xFE) return false;
		// AKA: reset next-link / trigger / mask registers
		node.CLAR = 0;
		node.CTBR = 0;
		node.CMAR = 0;
		node.CMDR = 0;
		node.Reserved = 0;
		// write CTCR
		stduint sinc, sincos, dinc, dincos;
		_MDMA_mapInc(conf.srcInc, sinc, sincos);
		_MDMA_mapInc(conf.dstInc, dinc, dincos);
		stduint ctcr = 0;
		ctcr |= (sinc << _MDMA_CTCR_POS_SINC) | (dinc << _MDMA_CTCR_POS_DINC);
		ctcr |= (sincos << _MDMA_CTCR_POS_SINCOS) | (dincos << _MDMA_CTCR_POS_DINCOS);
		ctcr |= ((stduint)conf.srcSize << _MDMA_CTCR_POS_SSIZE) | ((stduint)conf.dstSize << _MDMA_CTCR_POS_DSIZE);
		if (conf.align == MDMAAlign::PackEnable) ctcr |= (1U << _MDMA_CTCR_POS_PKE);
		else ctcr |= ((stduint)conf.align << _MDMA_CTCR_POS_PAM);
		ctcr |= ((stduint)conf.srcBurst << _MDMA_CTCR_POS_SBURST) | ((stduint)conf.dstBurst << _MDMA_CTCR_POS_DBURST);
		ctcr |= ((conf.bufferLen - 1) << _MDMA_CTCR_POS_TLEN);
		ctcr |= ((stduint)conf.trigger << _MDMA_CTCR_POS_TRGM);
		// AKA: SW request sets SWRM; SW request or mask address sets BWM
		if (conf.request == MDMA_REQUEST_SW) ctcr |= (1U << _MDMA_CTCR_POS_SWRM);
		if ((conf.request == MDMA_REQUEST_SW) || (conf.postMaskAddr != 0)) ctcr |= (1U << _MDMA_CTCR_POS_BWM);
		node.CTCR = ctcr;
		// write CBNDTR (block count + block offset sign + block data length) and CBRUR
		stduint cbndtr = ((conf.blockCount - 1) & 0xFFFU) << _MDMA_CBNDTR_POS_BRC;
		stduint cbrur = 0;
		if (conf.srcBlockOffset < 0) {
			cbndtr |= (1U << _MDMA_CBNDTR_POS_BRSUM);
			cbrur |= ((stduint)(-conf.srcBlockOffset) & 0xFFFFU);
		} else {
			cbrur |= ((stduint)conf.srcBlockOffset & 0xFFFFU);
		}
		if (conf.dstBlockOffset < 0) {
			cbndtr |= (1U << _MDMA_CBNDTR_POS_BRDUM);
			cbrur |= (((stduint)(-conf.dstBlockOffset) & 0xFFFFU) << _MDMA_CBRUR_POS_DUV);
		} else {
			cbrur |= (((stduint)conf.dstBlockOffset & 0xFFFFU) << _MDMA_CBRUR_POS_DUV);
		}
		cbndtr |= conf.blockDataLength;
		node.CBNDTR = cbndtr;
		node.CBRUR = cbrur;
		node.CDAR = _IMM(conf.dstAddr);
		node.CSAR = _IMM(conf.srcAddr);
		// AKA: HW request sets TSEL + post-request mask; SW request leaves CTBR/CMAR/CMDR reset
		if (conf.request != MDMA_REQUEST_SW) {
			node.CTBR = conf.request & 0x3FU;
			node.CMAR = _IMM(conf.postMaskAddr);
			node.CMDR = conf.postMaskData;
		}
		// AKA: bus selection (0x2000xxxx or 0x00xxxxxx -> AHB bus)
		stduint srcMask = _IMM(conf.srcAddr) & 0xFF000000U;
		if ((srcMask == 0x20000000U) || (srcMask == 0x00000000U)) node.CTBR |= (1U << _MDMA_CTBR_POS_SBUS);
		stduint dstMask = _IMM(conf.dstAddr) & 0xFF000000U;
		if ((dstMask == 0x20000000U) || (dstMask == 0x00000000U)) node.CTBR |= (1U << _MDMA_CTBR_POS_DBUS);
		return true;
	}

	// AKA HAL_MDMA_LinkedList_AddNode
	bool MDMAChannel::AddNode(MDMA_LinkNode& node, MDMA_LinkNode* prevNode) const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		if (p.channelStates[id] != _MDMA_STATE_READY) return false;
		p.channelStates[id] = _MDMA_STATE_BUSY;
		bool ok = true;
		if (p.firstNode[id] == 0) {
			// first node after the init node 0
			if (prevNode == 0) {
				self[Chan::CLAR] = _IMM(&node);
				p.firstNode[id] = &node;
				node.CLAR = 0;
				p.lastNode[id] = &node;
				p.nodeCounter[id] = 1;
			} else {
				ok = false;
			}
		} else if (p.firstNode[id] != &node) {
			// AKA duplicate check: new node already in the list (and not first)
			MDMA_LinkNode* n = p.firstNode[id];
			byte counter = 0;
			while ((counter < p.nodeCounter[id]) && ok) {
				if (n->CLAR == _IMM(&node)) ok = false;
				n = (MDMA_LinkNode*)_IMM(n->CLAR);
				counter++;
			}
			if (ok) {
				if ((prevNode == p.lastNode[id]) || (prevNode == 0)) {
					// append at end
					node.CLAR = p.lastNode[id]->CLAR;
					p.lastNode[id]->CLAR = _IMM(&node);
					p.lastNode[id] = &node;
					p.nodeCounter[id]++;
				} else {
					// insert after prevNode
					n = p.firstNode[id];
					counter = 0;
					bool inserted = false;
					while ((counter < p.nodeCounter[id]) && !inserted) {
						counter++;
						if (n == prevNode) {
							node.CLAR = n->CLAR;
							n->CLAR = _IMM(&node);
							p.nodeCounter[id]++;
							inserted = true;
						} else {
							n = (MDMA_LinkNode*)_IMM(n->CLAR);
						}
					}
					if (!inserted) ok = false;
				}
			}
		} else {
			// node is already the first node
			ok = false;
		}
		p.channelStates[id] = _MDMA_STATE_READY;
		return ok;
	}

	// AKA HAL_MDMA_LinkedList_RemoveNode
	bool MDMAChannel::RemoveNode(MDMA_LinkNode& node) const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		if (p.channelStates[id] != _MDMA_STATE_READY) return false;
		p.channelStates[id] = _MDMA_STATE_BUSY;
		bool ok = true;
		if ((p.firstNode[id] == 0) || (p.lastNode[id] == 0) || (p.nodeCounter[id] == 0)) {
			ok = false;
		} else if (p.firstNode[id] == &node) {
			// delete first node
			if (p.lastNode[id] == &node) {
				// single node
				p.firstNode[id] = 0;
				p.lastNode[id] = 0;
				p.nodeCounter[id] = 0;
				self[Chan::CLAR] = 0;
			} else {
				// AKA: circular list -> reconnect last node
				if (p.firstNode[id] == (MDMA_LinkNode*)_IMM(p.lastNode[id]->CLAR))
					p.lastNode[id]->CLAR = node.CLAR;
				self[Chan::CLAR] = node.CLAR;
				p.firstNode[id] = (MDMA_LinkNode*)_IMM(self[Chan::CLAR]);
				p.nodeCounter[id]--;
			}
		} else {
			// find and delete a non-first node
			MDMA_LinkNode* tmp = p.firstNode[id];
			byte counter = 0;
			bool deleted = false;
			while ((counter < p.nodeCounter[id]) && !deleted) {
				counter++;
				if (tmp->CLAR == _IMM(&node)) {
					if (&node == p.lastNode[id]) p.lastNode[id] = tmp;
					tmp->CLAR = node.CLAR;
					deleted = true;
					p.nodeCounter[id]--;
				} else {
					tmp = (MDMA_LinkNode*)_IMM(tmp->CLAR);
				}
			}
			if (!deleted) ok = false;
		}
		p.channelStates[id] = _MDMA_STATE_READY;
		return ok;
	}

	// AKA HAL_MDMA_LinkedList_EnableCircularMode / DisableCircularMode (unified)
	bool MDMAChannel::enCircular(bool ena) const {
		using namespace MDMAReg;
		byte id = getID();
		MDMA_t& p = getParent();
		if (p.channelStates[id] != _MDMA_STATE_READY) return false;
		p.channelStates[id] = _MDMA_STATE_BUSY;
		bool ok = true;
		if ((p.firstNode[id] == 0) || (p.lastNode[id] == 0) || (p.nodeCounter[id] == 0)) {
			ok = false;
		} else {
			// connect/disconnect last node to first
			p.lastNode[id]->CLAR = ena ? _IMM(p.firstNode[id]) : 0;
		}
		p.channelStates[id] = _MDMA_STATE_READY;
		return ok;
	}

#endif // _MCU_STM32H7x
}
