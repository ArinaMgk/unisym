// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Device) Serial Audio Interface, SAI
// Codifiers: @dosconio: 2025xxxx~;
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: RCC
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


#include "../../../inc/c/driver/SAI.h"
#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
#include "../../../inc/cpp/Device/SysTick"
#include "../../../inc/cpp/Device/DMA"
#include "../../../lib/cpp/Device/Interrupt/interrupt_sai.hpp"
#endif

namespace uni {
#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

#if defined(_MCU_STM32H7x)
	static const stduint sai_addr[2] = {
		D2_APB2PERIPH_BASE + 0x5800,// SAI1
		D2_APB2PERIPH_BASE + 0x5C00,// SAI2
	};
#elif defined(_MPU_STM32MP13)
	static const stduint sai_addr[2] = {
		APB2_PERIPH_BASE + 0xA000,// SAI1
		APB2_PERIPH_BASE + 0xB000,// SAI2
	};
#endif

	// SAI kernel clock frequency for Mckdiv computation
	static stduint sai_kernel_clock(byte id) {
	#if defined(_MCU_STM32H7x)
		return (id == 1) ? RCC.getPeriphClockFreq(PeriphClock::SAI1)
		                 : RCC.getPeriphClockFreq(PeriphClock::SAI23);
	#elif defined(_MPU_STM32MP13)
		return (id == 1) ? RCC.getPeriphClockFreq(PeriphClock::SAI1)
		                 : RCC.getPeriphClockFreq(PeriphClock::SAI2);
	#endif
	}

	// AKA HAL_SAI_MspInit clock enable
	bool SAI_t::enClock(bool ena) {
	#if defined(_MCU_STM32H7x)
		auto reg = RCCReg::APB2ENR; byte bit = (id == 1) ? 22 : 23;
		RCC[reg].setof(bit, ena);
		return RCC[reg].bitof(bit) == ena;
	#elif defined(_MPU_STM32MP13)
		byte bit = (id == 1) ? 16 : 17;
		RCC[ena ? RCCReg::MP_APB2ENSETR : RCCReg::MP_APB2ENCLRR] = _IMM1S(bit);
		return true;
	#endif
	}

	SAI_t& SAI_Global::operator[](byte id) {
		return (id == 2) ? SAI2 : SAI1;
	}

	Reference SAI_Block::operator[](SAIReg reg) {
		return blockBase() + _IMM(reg);
	}

	bool SAI_Block::enAble(bool ena) {
		self[SAIReg::CR1].setof(SAI_xCR1_SAIEN_Pos, ena);
		return true;
	}

	// data unit bytes per transfer (AKA HAL data-size branching)
	byte SAI_Block::dataUnitBytes() const {
		if (datasize == SAIDataSize_E::Data8 && comp == SAICompanding_E::None) return 1;
		if (_IMM(datasize) <= _IMM(SAIDataSize_E::Data16)) return 2;
		return 4;
	}

	// AKA SAI_InitI2S / SAI_InitPCM (InitProtocol: standard -> auto frame/slot)
	bool SAI_Block::applyProtocolStandard() {
		if (protocol == SAIProtocol_E::I2S || protocol == SAIProtocol_E::MsbJustified || protocol == SAIProtocol_E::LsbJustified) {
			first_bit = SAIFirstBit_E::MSB;
			// ClockStrobing per AudioMode (TX falling / RX rising)
			strobing = (audio_mode == SAIAudioMode_E::MasterTx || audio_mode == SAIAudioMode_E::SlaveTx)
				? SAIClockStrobing_E::FallingEdge : SAIClockStrobing_E::RisingEdge;
			fsdef = SAIFSDefinition_E::ChannelIdentification;
			slot_active = 0xFFFFFFFF;// SAI_SLOTACTIVE_ALL
			firstbit_offset = 0;
			if (slot_number & 0x1) return false;// I2S slot number must be even
			if (protocol == SAIProtocol_E::I2S) {
				fspol = SAIFSPolarity_E::ActiveLow;
				fsoff = SAIFSOffset_E::BeforeFirstBit;
			} else {
				fspol = SAIFSPolarity_E::ActiveHigh;
				fsoff = SAIFSOffset_E::FirstBit;
			}
			switch (_IMM(datasize)) {
			case _IMM(SAIDataSize_E::Data16):
				frame_length = 32 * (slot_number / 2);
				active_frame_length = 16 * (slot_number / 2);
				slotsize = SAISlotSize_E::DataSize;
				break;
			case _IMM(SAIDataSize_E::Data24):
				frame_length = 64 * (slot_number / 2);
				active_frame_length = 32 * (slot_number / 2);
				slotsize = SAISlotSize_E::B32;
				break;
			case _IMM(SAIDataSize_E::Data32):
				frame_length = 64 * (slot_number / 2);
				active_frame_length = 32 * (slot_number / 2);
				slotsize = SAISlotSize_E::B32;
				break;
			default:
				return false;
			}
			if (protocol == SAIProtocol_E::LsbJustified) {
				if (datasize == SAIDataSize_E::Data24) firstbit_offset = 8;
			}
		} else if (protocol == SAIProtocol_E::PcmLong || protocol == SAIProtocol_E::PcmShort) {
			first_bit = SAIFirstBit_E::MSB;
			strobing = (audio_mode == SAIAudioMode_E::MasterTx || audio_mode == SAIAudioMode_E::SlaveTx)
				? SAIClockStrobing_E::RisingEdge : SAIClockStrobing_E::FallingEdge;
			fsdef = SAIFSDefinition_E::StartFrame;
			fspol = SAIFSPolarity_E::ActiveHigh;
			fsoff = SAIFSOffset_E::BeforeFirstBit;
			firstbit_offset = 0;
			slot_active = 0xFFFFFFFF;
			active_frame_length = (protocol == SAIProtocol_E::PcmShort) ? 1 : 13;
			switch (_IMM(datasize)) {
			case _IMM(SAIDataSize_E::Data16):
				frame_length = 16 * slot_number;
				slotsize = SAISlotSize_E::DataSize;
				break;
			case _IMM(SAIDataSize_E::Data24):
				frame_length = 32 * slot_number;
				slotsize = SAISlotSize_E::B32;
				break;
			case _IMM(SAIDataSize_E::Data32):
				frame_length = 32 * slot_number;
				slotsize = SAISlotSize_E::B32;
				break;
			default:
				return false;
			}
		}
		return true;
	}

	// AKA HAL_SAI_Init
	bool SAI_Block::canMode() {
		uint32 tmpgcr = 0;
		uint32 syncen = 0;
		uint32 ckstr = 0;

		// auto frame/slot config for standard protocols (AKA HAL_SAI_InitProtocol)
		if (protocol >= SAIProtocol_E::I2S) {
			if (!applyProtocolStandard()) { error_code |= _SAI_ERR_NOT_SUPPORTED; return false; }
		}

		state = SAIXState::Busy;
		disableSai();

		// SAI PDM configuration (peripheral-level PDMCR)
		Reference(sai_base + _IMM(SAIGlobalReg::PDMCR)).rstof(SAI_PDMCR_PDMEN_Pos);
		if (pdm_act) {
			// PDM requires Block A + Master Rx + Free protocol
			if (block != 1 || audio_mode != SAIAudioMode_E::MasterRx) {
				error_code |= _SAI_ERR_NOT_SUPPORTED;
				return false;
			}
			Reference(sai_base + _IMM(SAIGlobalReg::PDMCR))
				= _IMM(clken) | ((mic_pairs - 1) << SAI_PDMCR_MICNBR_Pos);
			Reference(sai_base + _IMM(SAIGlobalReg::PDMCR)).setof(SAI_PDMCR_PDMEN_Pos);
		}

		// SAI block synchro (GCR)
		switch (synchro_ext) {
		case SAISynchroExt_E::Disable: tmpgcr = 0; break;
		case SAISynchroExt_E::OutBlockA: tmpgcr = SAI_GCR_SYNCOUT_0; break;
		case SAISynchroExt_E::OutBlockB: tmpgcr = SAI_GCR_SYNCOUT_1; break;
		default: break;
		}
		switch (synchro) {
		case SAISynchro_E::Asynchronous: syncen = 0; break;
		case SAISynchro_E::Synchronous: syncen = SAI_xCR1_SYNCEN_0; break;
		case SAISynchro_E::SynchronousExtSai1: syncen = SAI_xCR1_SYNCEN_1; break;
		case SAISynchro_E::SynchronousExtSai2:
			syncen = SAI_xCR1_SYNCEN_1;
			tmpgcr |= SAI_GCR_SYNCIN_0;
			break;
		default: break;
		}
		Reference(sai_base + _IMM(SAIGlobalReg::GCR)) = tmpgcr;

		// Mckdiv prescaler (AKA HAL formula)
		if (audio_freq != 0) {
			uint32 freq = sai_kernel_clock(id);
			uint32 tmpval;
			if (nodiv == SAINoDivider_E::NoDivider) {
				// NOMCK = 1: MCKDIV = CK / (FS * FRL)
				tmpval = (freq * 10) / (audio_freq * frame_length);
			} else {
				// NOMCK = 0: MCKDIV = CK / (FS * OSR * 256)
				uint32 tmposr = (mckosr == SAIMckOverSampling_E::Enable) ? 2 : 1;
				tmpval = (freq * 10) / (audio_freq * tmposr * 256);
			}
			mckdiv = tmpval / 10;
			if ((tmpval % 10) > 8) mckdiv += 1;
		}

		// CKSTR bits (per AudioMode + ClockStrobing)
		if (audio_mode == SAIAudioMode_E::MasterTx || audio_mode == SAIAudioMode_E::SlaveTx) {
			ckstr = (strobing == SAIClockStrobing_E::RisingEdge) ? 0 : SAI_xCR1_CKSTR_Msk;
		} else {
			ckstr = (strobing == SAIClockStrobing_E::RisingEdge) ? SAI_xCR1_CKSTR_Msk : 0;
		}

		// CR1
		self[SAIReg::CR1].maset(SAI_xCR1_MODE_Pos, 2, _IMM(audio_mode));
		self[SAIReg::CR1].maset(SAI_xCR1_PRTCFG_Pos, 2, _prtcfg());
		self[SAIReg::CR1].maset(SAI_xCR1_DS_Pos, 3, _IMM(datasize) >> SAI_xCR1_DS_Pos);
		self[SAIReg::CR1].setof(SAI_xCR1_LSBFIRST_Pos, first_bit == SAIFirstBit_E::LSB);
		self[SAIReg::CR1].setof(SAI_xCR1_CKSTR_Pos, ckstr);
		self[SAIReg::CR1].maset(SAI_xCR1_SYNCEN_Pos, 2, syncen >> SAI_xCR1_SYNCEN_Pos);
		self[SAIReg::CR1].setof(SAI_xCR1_MONO_Pos, mono == SAIMonoStereo_E::Mono);
		self[SAIReg::CR1].setof(SAI_xCR1_OUTDRIV_Pos, outdrive == SAIOutputDrive_E::Enable);
		self[SAIReg::CR1].setof(SAI_xCR1_NOMCK_Pos, nodiv == SAINoDivider_E::NoDivider);
		self[SAIReg::CR1].maset(SAI_xCR1_MCKDIV_Pos, 6, mckdiv);
	#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
		self[SAIReg::CR1].setof(SAI_xCR1_OSR_Pos, mckosr == SAIMckOverSampling_E::Enable);
	#endif
	#if defined(_MPU_STM32MP13)
		self[SAIReg::CR1].setof(SAI_xCR1_MCKEN_Pos, mckout == SAIMckOutput_E::Enable);
	#endif

		// CR2
		self[SAIReg::CR2].maset(SAI_xCR2_FTH_Pos, 3, _IMM(fth));
		self[SAIReg::CR2].maset(SAI_xCR2_COMP_Pos, 2, _IMM(comp) >> SAI_xCR2_COMP_Pos);
		self[SAIReg::CR2].setof(SAI_xCR2_TRIS_Pos, tristate == SAITriState_E::Enable);

		// FRCR
		self[SAIReg::FRCR].maset(SAI_xFRCR_FRL_Pos, 8, frame_length - 1);
		self[SAIReg::FRCR].maset(SAI_xFRCR_FSALL_Pos, 7, active_frame_length - 1);
		self[SAIReg::FRCR].setof(SAI_xFRCR_FSDEF_Pos, fsdef == SAIFSDefinition_E::ChannelIdentification);
		self[SAIReg::FRCR].setof(SAI_xFRCR_FSPOL_Pos, fspol == SAIFSPolarity_E::ActiveHigh);
		self[SAIReg::FRCR].setof(SAI_xFRCR_FSOFF_Pos, fsoff == SAIFSOffset_E::BeforeFirstBit);

		// SLOTR
		self[SAIReg::SLOTR].maset(SAI_xSLOTR_FBOFF_Pos, 5, firstbit_offset);
		self[SAIReg::SLOTR].maset(SAI_xSLOTR_SLOTSZ_Pos, 2, _IMM(slotsize) >> SAI_xSLOTR_SLOTSZ_Pos);
		self[SAIReg::SLOTR].maset(SAI_xSLOTR_NBSLOT_Pos, 4, slot_number - 1);
		self[SAIReg::SLOTR].maset(SAI_xSLOTR_SLOTEN_Pos, 16, slot_active);

		error_code = _SAI_ERR_NONE;
		state = SAIXState::Ready;
		return true;
	}

	// PRTCFG[1:0] value for the abstract protocol
	stduint SAI_Block::_prtcfg() const {
		switch (protocol) {
		case SAIProtocol_E::Spdif: return 0x1;
		case SAIProtocol_E::Ac97: return 0x2;
		default: return 0x0;// Free / I2S / Msb / Lsb / Pcm -> FREE protocol
		}
	}

	// AKA HAL_SAIEx_ConfigPdmMicDelay (PDMDLY: per-pair left/right delay)
	bool SAI_Block::setPdmMicDelay(stduint mic_pair, stduint left_delay, stduint right_delay) {
		if (mic_pair < 1 || mic_pair > 4) return false;
		stduint shift = 8 * (mic_pair - 1);
		Reference pdmdly(sai_base + _IMM(SAIGlobalReg::PDMDLY));
		pdmdly &= ~(0x77U << shift);
		pdmdly |= (((right_delay & 0x7) << 4) | (left_delay & 0x7)) << shift;
		return true;
	}

	// AKA SAI_Disable (clear SAIEN, wait until cleared)
	void SAI_Block::disableSai() {
		enAble(false);
		uint64 tickstart = SysTick::getTick();
		while (self[SAIReg::CR1].bitof(SAI_xCR1_SAIEN_Pos)) {
			if ((SysTick::getTick() - tickstart) > _SAI_TIMEOUT_VALUE) {
				error_code |= _SAI_ERR_TIMEOUT;
				break;
			}
		}
	}

	// AKA SAI_FillFifo
	void SAI_Block::fillFifo() {
		byte unit = dataUnitBytes();
		while (((self[SAIReg::SR].masof(SAI_xSR_FLVL_Pos, 3)) != 5) && (tx_count > 0)) {
			if (unit == 1) { self[SAIReg::DR] = *tx_buff++; }
			else if (unit == 2) { self[SAIReg::DR] = *(const uint16*)tx_buff; tx_buff += 2; }
			else { self[SAIReg::DR] = *(const uint32*)tx_buff; tx_buff += 4; }
			tx_count--;
		}
	}

	// AKA HAL_SAI_Transmit / _IT / _DMA
	stduint SAI_Block::Transmit(const byte* tx, stduint size, IOMethod method) {
		if (!tx || !size) return 0;
		byte unit = dataUnitBytes();
		switch (method) {
		case IOMethod::Rupt: {
			if (state != SAIXState::Ready) return 0;
			state = SAIXState::BusyTX; error_code = _SAI_ERR_NONE;
			tx_buff = tx; tx_size = size; tx_count = size;
			rx_buff = nullptr; rx_size = 0; rx_count = 0;
			fillFifo();
			self[SAIReg::IMR] = SAI_xSR_FREQ_Msk | SAI_xSR_OVRUDR_Msk | _itErrorFlags();
			enAble(true);
			return size;
		}
		case IOMethod::DMA: {
			if (state != SAIXState::Ready) return 0;
			state = SAIXState::BusyTX; error_code = _SAI_ERR_NONE;
			tx_buff = tx; tx_size = size; tx_count = size;
			rx_buff = nullptr; rx_size = 0; rx_count = 0;
			self[SAIReg::CR1].setof(SAI_xCR1_DMAEN_Pos);
			self[SAIReg::IMR] = SAI_xSR_OVRUDR_Msk | _itErrorFlags();
			enAble(true);
			if (!dmaTxStart((pureptr_t)tx, (pureptr_t)(blockBase() + _IMM(SAIReg::DR)), size)) {
				self[SAIReg::CR1].rstof(SAI_xCR1_DMAEN_Pos);
				state = SAIXState::Ready; return 0;
			}
			return size;
		}
		case IOMethod::Loop: break;
		}
		// Loop
		if (state != SAIXState::Ready) return 0;
		state = SAIXState::BusyTX; error_code = _SAI_ERR_NONE;
		tx_buff = tx; tx_size = size; tx_count = size;
		rx_buff = nullptr; rx_size = 0; rx_count = 0;
		fillFifo();
		enAble(true);
		uint64 tickstart = SysTick::getTick();
		while (tx_count > 0) {
			if (self[SAIReg::SR].masof(SAI_xSR_FLVL_Pos, 3) != 5) {
				if (unit == 1) { self[SAIReg::DR] = *tx_buff++; }
				else if (unit == 2) { self[SAIReg::DR] = *(const uint16*)tx_buff; tx_buff += 2; }
				else { self[SAIReg::DR] = *(const uint32*)tx_buff; tx_buff += 4; }
				tx_count--;
			} else if ((SysTick::getTick() - tickstart) > _SAI_TIMEOUT_VALUE) {
				error_code |= _SAI_ERR_TIMEOUT;
				self[SAIReg::CLRFR] = 0xFFFFFFFF;
				disableSai();
				self[SAIReg::CR2].setof(SAI_xCR2_FFLUSH_Pos);
				state = SAIXState::Ready;
				return 0;
			}
		}
		state = SAIXState::Ready;
		return error_code ? 0 : size;
	}

	// AKA HAL_SAI_Receive / _IT / _DMA
	stduint SAI_Block::Receive(byte* rx, stduint size, IOMethod method) {
		if (!rx || !size) return 0;
		byte unit = dataUnitBytes();
		switch (method) {
		case IOMethod::Rupt: {
			if (state != SAIXState::Ready) return 0;
			state = SAIXState::BusyRX; error_code = _SAI_ERR_NONE;
			rx_buff = rx; rx_size = size; rx_count = size;
			tx_buff = nullptr; tx_size = 0; tx_count = 0;
			self[SAIReg::IMR] = SAI_xSR_FREQ_Msk | SAI_xSR_OVRUDR_Msk | _itErrorFlags();
			enAble(true);
			return size;
		}
		case IOMethod::DMA: {
			if (state != SAIXState::Ready) return 0;
			state = SAIXState::BusyRX; error_code = _SAI_ERR_NONE;
			rx_buff = rx; rx_size = size; rx_count = size;
			tx_buff = nullptr; tx_size = 0; tx_count = 0;
			self[SAIReg::CR1].setof(SAI_xCR1_DMAEN_Pos);
			self[SAIReg::IMR] = SAI_xSR_OVRUDR_Msk | _itErrorFlags();
			enAble(true);
			if (!dmaRxStart((pureptr_t)(blockBase() + _IMM(SAIReg::DR)), (pureptr_t)rx, size)) {
				self[SAIReg::CR1].rstof(SAI_xCR1_DMAEN_Pos);
				state = SAIXState::Ready; return 0;
			}
			return size;
		}
		case IOMethod::Loop: break;
		}
		// Loop
		if (state != SAIXState::Ready) return 0;
		state = SAIXState::BusyRX; error_code = _SAI_ERR_NONE;
		rx_buff = rx; rx_size = size; rx_count = size;
		tx_buff = nullptr; tx_size = 0; tx_count = 0;
		enAble(true);
		uint64 tickstart = SysTick::getTick();
		while (rx_count > 0) {
			if (self[SAIReg::SR].masof(SAI_xSR_FLVL_Pos, 3) != 0) {
				if (unit == 1) { *rx_buff++ = (byte)self[SAIReg::DR]; }
				else if (unit == 2) { *(uint16*)rx_buff = (uint16)self[SAIReg::DR]; rx_buff += 2; }
				else { *(uint32*)rx_buff = self[SAIReg::DR]; rx_buff += 4; }
				rx_count--;
			} else if ((SysTick::getTick() - tickstart) > _SAI_TIMEOUT_VALUE) {
				error_code |= _SAI_ERR_TIMEOUT;
				self[SAIReg::CLRFR] = 0xFFFFFFFF;
				disableSai();
				self[SAIReg::CR2].setof(SAI_xCR2_FFLUSH_Pos);
				state = SAIXState::Ready;
				return 0;
			}
		}
		state = SAIXState::Ready;
		return error_code ? 0 : size;
	}

	// error interrupt flags per mode (AKA SAI_InterruptFlag)
	stduint SAI_Block::_itErrorFlags() const {
		stduint flags = 0;
		if (audio_mode == SAIAudioMode_E::SlaveRx || audio_mode == SAIAudioMode_E::SlaveTx)
			flags |= SAI_xSR_AFSDET_Msk | SAI_xSR_LFSDET_Msk;
		else
			flags |= SAI_xSR_WCKCFG_Msk;
		return flags;
	}

	// AKA HAL_SAI_DMAPause
	bool SAI_Block::PauseDMA() {
		self[SAIReg::CR1].rstof(SAI_xCR1_DMAEN_Pos);
		return true;
	}
	// AKA HAL_SAI_DMAResume
	bool SAI_Block::ResumeDMA() {
		self[SAIReg::CR1].setof(SAI_xCR1_DMAEN_Pos);
		return true;
	}
	// AKA HAL_SAI_DMAStop
	bool SAI_Block::StopDMA() {
		self[SAIReg::CR1].rstof(SAI_xCR1_DMAEN_Pos);
		if (hdmatx) hdmatx->Abort();
		if (hdmarx) hdmarx->Abort();
		state = SAIXState::Ready;
		return true;
	}
	// AKA HAL_SAI_Abort
	bool SAI_Block::Abort() {
		self[SAIReg::IMR] = 0;
		self[SAIReg::CLRFR] = 0xFFFFFFFF;
		if (hdmatx) hdmatx->Abort();
		if (hdmarx) hdmarx->Abort();
		disableSai();
		self[SAIReg::CR2].setof(SAI_xCR2_FFLUSH_Pos);
		tx_count = 0; rx_count = 0;
		state = SAIXState::Ready;
		return true;
	}

	// AKA HAL_SAI_EnableTxMuteMode
	bool SAI_Block::EnableTxMuteMode(stduint val) {
		if (state != SAIXState::Ready) return false;
		state = SAIXState::Busy;
		self[SAIReg::CR2].maset(SAI_xCR2_MUTECNT_Pos, 6, 0);// reset MUTECNT
		self[SAIReg::CR2].setof(SAI_xCR2_MUTEVAL_Pos, val & 1);
		self[SAIReg::CR2].setof(SAI_xCR2_MUTE_Pos);
		state = SAIXState::Ready;
		return true;
	}
	// AKA HAL_SAI_DisableTxMuteMode
	bool SAI_Block::DisableTxMuteMode() {
		if (state != SAIXState::Ready) return false;
		state = SAIXState::Busy;
		self[SAIReg::CR2].rstof(SAI_xCR2_MUTE_Pos);
		state = SAIXState::Ready;
		return true;
	}
	// AKA HAL_SAI_EnableRxMuteMode
	bool SAI_Block::EnableRxMuteMode(Handler_t cb, stduint counter) {
		if (state != SAIXState::Ready) return false;
		state = SAIXState::Busy;
		mute_handler = cb;
		self[SAIReg::CR2].maset(SAI_xCR2_MUTECNT_Pos, 6, counter & 0x3F);
		self[SAIReg::CR2].setof(SAI_xCR2_MUTE_Pos);
		state = SAIXState::Ready;
		return true;
	}
	// AKA HAL_SAI_DisableRxMuteMode
	bool SAI_Block::DisableRxMuteMode() {
		if (state != SAIXState::Ready) return false;
		state = SAIXState::Busy;
		self[SAIReg::CR2].rstof(SAI_xCR2_MUTE_Pos);
		mute_handler = nullptr;
		state = SAIXState::Ready;
		return true;
	}

	// AKA HAL_SAI_IRQHandler (full dispatch: FREQ / OVRUDR / MUTEDET / AFSDET / LFSDET / WCKCFG / CNRDY)
	void SAI_Block::irqHandler() {
		if (state == SAIXState::Reset) return;
		uint32 itflags = self[SAIReg::SR];
		uint32 itsrc = self[SAIReg::IMR];
		uint32 trigger = itflags & itsrc;

		if (trigger & SAI_xSR_FREQ_Msk) {
			if (state == SAIXState::BusyTX) outHandlerByInterrupt();
			else if (state == SAIXState::BusyRX) innHandlerByInterrupt();
		} else if (trigger & SAI_xSR_OVRUDR_Msk) {
			self[SAIReg::CLRFR] = SAI_xSR_OVRUDR_Msk;
			setError(state == SAIXState::BusyRX ? _SAI_ERR_OVR : _SAI_ERR_UDR);
			if (ErrorHandler) ErrorHandler();
		} else if (trigger & SAI_xSR_MUTEDET_Msk) {
			self[SAIReg::CLRFR] = SAI_xSR_MUTEDET_Msk;
			if (mute_handler) mute_handler();
		} else if (trigger & SAI_xSR_AFSDET_Msk) {
			setError(_SAI_ERR_AFSDET);
			closeTransfer();
			if (ErrorHandler) ErrorHandler();
		} else if (trigger & SAI_xSR_LFSDET_Msk) {
			setError(_SAI_ERR_LFSDET);
			closeTransfer();
			if (ErrorHandler) ErrorHandler();
		} else if (trigger & SAI_xSR_WCKCFG_Msk) {
			setError(_SAI_ERR_WCKCFG);
			closeTransfer();
			if (ErrorHandler) ErrorHandler();
		} else if (trigger & SAI_xSR_CNRDY_Msk) {
			self[SAIReg::CLRFR] = SAI_xSR_CNRDY_Msk;
			setError(_SAI_ERR_CNREADY);
			if (ErrorHandler) ErrorHandler();
		}
	}

	// AKA HAL_SAI_IRQHandler (RX branch) + SAI_Receive_IT8/16/32Bit
	void SAI_Block::innHandlerByInterrupt() {
		byte unit = dataUnitBytes();
		while (self[SAIReg::SR].bitof(SAI_xSR_FREQ_Pos) && rx_count > 0) {
			if (unit == 1) { *rx_buff++ = (byte)self[SAIReg::DR]; }
			else if (unit == 2) { *(uint16*)rx_buff = (uint16)self[SAIReg::DR]; rx_buff += 2; }
			else { *(uint32*)rx_buff = self[SAIReg::DR]; rx_buff += 4; }
			rx_count--;
		}
		if (rx_count == 0) {
			self[SAIReg::IMR].rstof(SAI_xSR_FREQ_Pos);
			state = SAIXState::Ready;
			if (RxCpltHandler) RxCpltHandler();
		}
	}

	// AKA HAL_SAI_IRQHandler (TX branch) + SAI_Transmit_IT8/16/32Bit
	void SAI_Block::outHandlerByInterrupt() {
		byte unit = dataUnitBytes();
		while (self[SAIReg::SR].bitof(SAI_xSR_FREQ_Pos) && tx_count > 0) {
			if (unit == 1) { self[SAIReg::DR] = *tx_buff++; }
			else if (unit == 2) { self[SAIReg::DR] = *(const uint16*)tx_buff; tx_buff += 2; }
			else { self[SAIReg::DR] = *(const uint32*)tx_buff; tx_buff += 4; }
			tx_count--;
		}
		if (tx_count == 0) {
			self[SAIReg::IMR].rstof(SAI_xSR_FREQ_Pos);
			state = SAIXState::Ready;
			if (TxCpltHandler) TxCpltHandler();
		}
	}

	// AKA HAL_SAI_IRQHandler error treatment (shared, called from interrupt_sai.hpp)
	void SAI_Block::closeTransfer() {
		self[SAIReg::IMR] = 0;
		self[SAIReg::CLRFR] = 0xFFFFFFFF;
		disableSai();
		self[SAIReg::CR2].setof(SAI_xCR2_FFLUSH_Pos);
		tx_count = 0; rx_count = 0;
		state = SAIXState::Ready;
	}

	// DMA completion (AKA SAI_DMATxCplt / SAI_DMARxCplt)
	void SAI_Block::dmaCpltISR() {
		SAIXState st = state;
		self[SAIReg::CR1].rstof(SAI_xCR1_DMAEN_Pos);
		tx_count = 0; rx_count = 0;
		state = SAIXState::Ready;
		if (error_code != _SAI_ERR_NONE) { if (ErrorHandler) ErrorHandler(); return; }
		if (st == SAIXState::BusyTX) { if (TxCpltHandler) TxCpltHandler(); }
		else if (st == SAIXState::BusyRX) { if (RxCpltHandler) RxCpltHandler(); }
	}

	// DMA completion dispatch (AKA SAI_DMATxCplt / SAI_DMARxCplt / SAI_DMAError)
	static void sai_dmaCplt() {
		SAI_Block* sp = nullptr;
		if (DMA1.XferCpltCallback == sai_dmaCplt) sp = (SAI_Block*)DMA1.bind;
		else if (DMA2.XferCpltCallback == sai_dmaCplt) sp = (SAI_Block*)DMA2.bind;
		if (!sp) return;
		sp->dmaCpltISR();
	}
	static void sai_dmaError() {
		SAI_Block* sp = nullptr;
		if (DMA1.XferErrorCallback == sai_dmaError) sp = (SAI_Block*)DMA1.bind;
		else if (DMA2.XferErrorCallback == sai_dmaError) sp = (SAI_Block*)DMA2.bind;
		if (!sp) return;
		sp->setError(_SAI_ERR_DMA);
		sp->dmaCpltISR();
	}

	// DMA start helpers (DMA1/DMA2 streams)
	bool SAI_Block::dmaTxStart(pureptr_t mem, pureptr_t peri, stduint ndtr) {
		if (!hdmatx) return false;
		DMA_t& dma = hdmatx->getParent();
		dma.bind = (pureptr_t)this;
		dma.XferCpltCallback = sai_dmaCplt;
		dma.XferErrorCallback = sai_dmaError;
		return hdmatx->Transfer(mem, peri, ndtr, IOMethod::Rupt);
	}
	bool SAI_Block::dmaRxStart(pureptr_t peri, pureptr_t mem, stduint ndtr) {
		if (!hdmarx) return false;
		DMA_t& dma = hdmarx->getParent();
		dma.bind = (pureptr_t)this;
		dma.XferCpltCallback = sai_dmaCplt;
		dma.XferErrorCallback = sai_dmaError;
		return hdmarx->Transfer(peri, mem, ndtr, IOMethod::Rupt);
	}

	SAI_t SAI1(1, sai_addr[0]), SAI2(2, sai_addr[1]);

#endif

}
