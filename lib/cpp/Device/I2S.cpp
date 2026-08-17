// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Protocol) Inter-IC Sound, I2S
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


#include "../../../inc/c/driver/I2S.h"
#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
#include "../../../inc/cpp/Device/SysTick"
#include "../../../inc/cpp/Device/DMA"
#endif

namespace uni {
#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

#if defined(_MCU_STM32H7x)
	static const stduint i2s_addr[3] = {
		D2_APB2PERIPH_BASE + 0x3000,// SPI1 (I2S1)
		D2_APB1PERIPH_BASE + 0x3800,// SPI2 (I2S2)
		D2_APB1PERIPH_BASE + 0x3C00,// SPI3 (I2S3)
	};
#elif defined(_MPU_STM32MP13)
	static const stduint i2s_addr[3] = {
		APB2_PERIPH_BASE + 0x4000,// SPI1 (I2S1)
		APB1_PERIPH_BASE + 0xB000,// SPI2 (I2S2)
		APB1_PERIPH_BASE + 0xC000,// SPI3 (I2S3)
	};
#endif

	// DataFormat -> I2SCFGR DATLEN/CHLEN register encoding (H7 and MP13 differ)
	static stduint i2s_df_bits(I2SDataFormat_E df) {
	#if defined(_MCU_STM32H7x)
		static const stduint tbl[4] = { 0x00000000, 0x00000400, 0x00000500, 0x00000600 };
	#elif defined(_MPU_STM32MP13)
		static const stduint tbl[4] = { 0x00000000, 0x00000400, 0x00000100, 0x00000200 };
	#endif
		return tbl[_IMM(df)];
	}

	Reference I2S_HARD::operator[](SPIReg reg) {
		return i2s_addr[id - 1] + _IMM(reg);
	}

	bool I2S_HARD::enClock(bool ena) {
	#if defined(_MCU_STM32H7x)
		asrtret(Ranglin(id, 1, 3));
		auto reg = RCCReg::APB1LENR; byte bit = 0;
		switch (id) {
		case 1: reg = RCCReg::APB2ENR; bit = 12; break;
		case 2: reg = RCCReg::APB1LENR; bit = 14; break;
		case 3: reg = RCCReg::APB1LENR; bit = 15; break;
		default: return false;
		}
		RCC[reg].setof(bit, ena);
		return RCC[reg].bitof(bit) == ena;
	#elif defined(_MPU_STM32MP13)
		asrtret(Ranglin(id, 1, 3));
		auto reg = RCCReg::MP_APB1ENSETR; byte bit = 0;
		switch (id) {
		case 1: reg = ena ? RCCReg::MP_APB2ENSETR : RCCReg::MP_APB2ENCLRR; bit = 8; break;
		case 2: reg = ena ? RCCReg::MP_APB1ENSETR : RCCReg::MP_APB1ENCLRR; bit = 11; break;
		case 3: reg = ena ? RCCReg::MP_APB1ENSETR : RCCReg::MP_APB1ENCLRR; bit = 12; break;
		default: return false;
		}
		RCC[reg] = _IMM1S(bit);
		return true;
	#endif
	}

	bool I2S_HARD::enAble(bool ena) {
		self[SPIReg::CR1].setof(SPI_CR1_SPE_Pos, ena);
		return true;
	}

	// AKA HAL_I2S_Init (register configuration + platform prescaler computation)
	bool I2S_HARD::canMode() {
		if (!enClock()) return false;
		stduint i2sdiv = 2, i2sodd = 0;

		// I2SCFGR must be edited with SPE off (both HALs disable first)
		if (self[SPIReg::CR1].bitof(SPI_CR1_SPE_Pos)) enAble(false);
		self[SPIReg::I2SCFGR] = 0;
		i2scfg_cur = 0;

	#if defined(_MCU_STM32H7x)
		// --- prescaler (computed for master & slave alike, AKA HAL_I2S_Init H7) ---
		if (audio_freq != 0) {
			stduint packetlength = (datafmt == I2SDataFormat_E::Bits16) ? 16 : 32;
			// Philips / MSB / LSB: packet length doubled (PCM not)
			if (_IMM(standard) <= 0x20) packetlength *= 2;
			stduint i2sclk = RCC.getPeriphClockFreq(PeriphClock::SPI123);// SPI1/2/3 share kernel clock
			stduint tmp;
			if (mclk == I2SMCLKOutput_E::Enable) {
				if (datafmt != I2SDataFormat_E::Bits16)
					tmp = (stduint)(((((i2sclk / (packetlength * 4)) * 10) / audio_freq)) + 5);
				else
					tmp = (stduint)(((((i2sclk / (packetlength * 8)) * 10) / audio_freq)) + 5);
			} else {
				tmp = (stduint)(((((i2sclk / packetlength) * 10) / audio_freq)) + 5);
			}
			tmp = tmp / 10;
			i2sodd = tmp & 1;
			i2sdiv = (tmp - i2sodd) / 2;
		}
		if ((i2sdiv < 2) || (i2sdiv > 0xFF)) {
			error_code |= _I2S_ERR_PRESCALER;
			return false;
		}
		// I2SCFGR: I2SMOD | standard | dataformat | cpol | fixch | wsinv | align24 | i2sdiv | odd | mclk
		// (I2SCFG direction field is written per-transfer by setDirection)
		self[SPIReg::I2SCFGR] = SPI_I2SCFGR_I2SMOD
			| _IMM(standard) | i2s_df_bits(datafmt) | _IMM(cpol) | _IMM(mclk)
			| (fixch == I2SSlaveExtendFREDetection_E::Enable ? SPI_I2SCFGR_FIXCH_Msk : 0)
			| (wsinv == I2SWSInversion_E::Enable ? SPI_I2SCFGR_WSINV_Msk : 0)
			| (align24 == I2SData24BitAlignment_E::Left ? SPI_I2SCFGR_DATFMT_Msk : 0)
			| (i2sdiv << SPI_I2SCFGR_I2SDIV_Pos) | (i2sodd << SPI_I2SCFGR_ODD_Pos);
		// CFG1: FIFO threshold (H7)
		self[SPIReg::CFG1].maset(SPI_CFG1_FTHLV_Pos, 4, _IMM(fifo_thr) >> SPI_CFG1_FTHLV_Pos);
		// CFG2: unlock AF, then LSBFRST | IOSWP; AFCNTR only for Master
		self[SPIReg::CR1].rstof(SPI_CR1_IOLOCK_Pos);
		self[SPIReg::CFG2].setof(SPI_CFG2_LSBFRST_Pos, first_bit == I2SFirstBit_E::LSB);
		self[SPIReg::CFG2].setof(SPI_CFG2_IOSWP_Pos, ioswap == I2SIOSwap_E::Enable);
		if (mode == I2SMode_E::Master)
			self[SPIReg::CFG2].setof(SPI_CFG2_AFCNTR_Pos, keep_io == I2SMasterKeepIO_E::Enable);
	#elif defined(_MPU_STM32MP13)
		// --- prescaler (Master only, AKA HAL_I2S_Init MP13) ---
		if (mode == I2SMode_E::Master && audio_freq != 0) {
			stduint packetlength = (datafmt == I2SDataFormat_E::Bits16) ? 1 : 2;
			bool ispcm = (standard == I2SStandard_E::PCM_Short) || (standard == I2SStandard_E::PCM_Long);
			stduint i2sclk = (id == 1) ? RCC.getPeriphClockFreq(PeriphClock::SPI1)
			                           : RCC.getPeriphClockFreq(PeriphClock::SPI23);
			stduint tmp;
			if (mclk == I2SMCLKOutput_E::Enable)
				tmp = (stduint)((((i2sclk / (256UL >> (ispcm ? 1 : 0))) * 10UL) / audio_freq) + 5UL);
			else
				tmp = (stduint)((((i2sclk / ((32UL >> (ispcm ? 1 : 0)) * packetlength)) * 10UL) / audio_freq) + 5UL);
			tmp = tmp / 10UL;
			i2sodd = tmp & 1UL;
			i2sdiv = (tmp - i2sodd) / 2UL;
		}
		if (((i2sodd == 1) && (i2sdiv == 1)) || (i2sdiv > 0xFF)) {
			error_code |= _I2S_ERR_PRESCALER;
			return false;
		}
		if (i2sdiv == 0) i2sodd = 1;
		if (mode == I2SMode_E::Master) {
			self[SPIReg::I2SCFGR].maset(SPI_I2SCFGR_I2SDIV_Pos, 8, i2sdiv);
			self[SPIReg::I2SCFGR].setof(SPI_I2SCFGR_ODD_Pos, i2sodd);
		}
		// I2SCFGR: I2SMOD | standard | dataformat | cpol | wsinv | align24 | mclk
		self[SPIReg::I2SCFGR] = SPI_I2SCFGR_I2SMOD
			| _IMM(standard) | i2s_df_bits(datafmt) | _IMM(cpol) | _IMM(mclk)
			| (wsinv == I2SWSInversion_E::Enable ? SPI_I2SCFGR_WSINV_Msk : 0)
			| (align24 == I2SData24BitAlignment_E::Left ? SPI_I2SCFGR_DATFMT_Msk : 0);
		// clear status register (AKA WRITE_REG(IFCR, 0x0FF8))
		self[SPIReg::IFCR] = 0x0FF8;
		// CFG2: unlock AF, then LSBFRST; AFCNTR only for Master
		self[SPIReg::CR1].rstof(SPI_CR1_IOLOCK_Pos);
		self[SPIReg::CFG2].setof(SPI_CFG2_LSBFRST_Pos, first_bit == I2SFirstBit_E::LSB);
		if (mode == I2SMode_E::Master)
			self[SPIReg::CFG2].setof(SPI_CFG2_AFCNTR_Pos, keep_io == I2SMasterKeepIO_E::Enable);
	#endif
		error_code = _I2S_ERR_NONE;
		state = I2SXState::Ready;
		return true;
	}

	// dir: 0=Tx 1=Rx 2=FD -> I2SCFG[2:0] (slave: 0/1/4, master: 2/3/5)
	void I2S_HARD::setDirection(byte dir) {
		static const byte enc[2][3] = { { 0, 1, 4 }, { 2, 3, 5 } };
		byte enc_v = enc[_IMM(mode) & 1][dir & 3];
		if (i2scfg_cur == enc_v) return;
		// I2SCFG must be edited with SPE off; restore after (same-direction streams skip this)
		bool was_on = self[SPIReg::CR1].bitof(SPI_CR1_SPE_Pos);
		if (was_on) enAble(false);
		self[SPIReg::I2SCFGR].maset(SPI_I2SCFGR_I2SCFG_Pos, 3, enc_v);
		i2scfg_cur = enc_v;
		if (was_on) enAble(true);
	}

	// AKA HAL_I2S_Transmit / _IT / _DMA
	stduint I2S_HARD::Transmit(const uint16* tx, stduint size, IOMethod method) {
		if (!tx || !size) return 0;
		const bool is32 = (datafmt == I2SDataFormat_E::Bits24) || (datafmt == I2SDataFormat_E::Bits32);
		volatile uint32* txdr = (volatile uint32*)(i2s_addr[id - 1] + _IMM(SPIReg::TXDR));
		switch (method) {
		case IOMethod::Rupt: {
			// AKA HAL_I2S_Transmit_IT
			if (state != I2SXState::Ready) return 0;
			state = I2SXState::BusyTX; error_code = _I2S_ERR_NONE;
			tx_buff = tx; tx_size = size; tx_count = size;
			rx_buff = nullptr; rx_size = 0; rx_count = 0;
			setDirection(0);
			enAble(true);
		#if defined(_MCU_STM32H7x)
			if (mode == I2SMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		#elif defined(_MPU_STM32MP13)
			self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		#endif
			self[SPIReg::IER] = SPI_IER_TXPIE | SPI_IER_UDRIE | SPI_IER_OVRIE | SPI_IER_TIFREIE;
			return size;
		}
		case IOMethod::DMA: {
			// AKA HAL_I2S_Transmit_DMA
			if (state != I2SXState::Ready) return 0;
			state = I2SXState::BusyTX; error_code = _I2S_ERR_NONE;
			tx_buff = tx; tx_size = size; tx_count = size;
			rx_buff = nullptr; rx_size = 0; rx_count = 0;
			setDirection(0);
			self[SPIReg::CFG1].setof(SPI_CFG1_TXDMAEN_Pos);
			enAble(true);
		#if defined(_MCU_STM32H7x)
			if (mode == I2SMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		#elif defined(_MPU_STM32MP13)
			self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		#endif
			if (!dmaTxStart((pureptr_t)tx, (pureptr_t)txdr, size)) {
				self[SPIReg::CFG1].rstof(SPI_CFG1_TXDMAEN_Pos);
				state = I2SXState::Ready; return 0;
			}
			return size;
		}
		case IOMethod::Loop: break;
		}
		// Loop (AKA HAL_I2S_Transmit)
		if (state != I2SXState::Ready) return 0;
		state = I2SXState::BusyTX; error_code = _I2S_ERR_NONE;
		tx_buff = tx; tx_size = size; tx_count = size;
		rx_buff = nullptr; rx_size = 0; rx_count = 0;
		setDirection(0);
		enAble(true);
	#if defined(_MCU_STM32H7x)
		if (mode == I2SMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
	#elif defined(_MPU_STM32MP13)
		self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
	#endif
		uint64 tickstart = SysTick::getTick();
		while (tx_count > 0) {
			if (self[SPIReg::SR].bitof(SPI_SR_TXP_Pos)) {
				if (is32) {
					*txdr = *(const uint32*)tx_buff; tx_buff += 2; tx_count--;
				}
			#if defined(_MCU_STM32H7x)
				// AKA H7 16-bit FIFO pack (FifoThreshold > 01DATA)
				else if (tx_count > 1 && _IMM(fifo_thr) > 0x00) {
					*txdr = *(const uint32*)tx_buff; tx_buff += 2; tx_count -= 2;
				}
			#endif
				else {
					*(volatile uint16*)txdr = *tx_buff; tx_buff += 1; tx_count--;
				}
			}
			else if ((SysTick::getTick() - tickstart) > _I2S_TIMEOUT_VALUE) {
				error_code |= _I2S_ERR_TIMEOUT; break;
			}
		#if defined(_MPU_STM32MP13)
			if (self[SPIReg::SR].bitof(SPI_SR_UDR_Pos)) {
				self[SPIReg::IFCR] = SPI_IFCR_UDRC;
				error_code |= _I2S_ERR_UDR;
			}
		#endif
		}
	#if defined(_MCU_STM32H7x)
		// AKA H7: wait TXE (TXP) cleared = TX FIFO drained
		if (!waitFlag(SPI_SR_TXP_Pos, false, tickstart, _I2S_TIMEOUT_VALUE)) error_code |= _I2S_ERR_TIMEOUT;
	#endif
		state = I2SXState::Ready;
		return (error_code & _I2S_ERR_TIMEOUT) ? 0 : size;
	}

	// AKA HAL_I2S_Receive / _IT / _DMA
	stduint I2S_HARD::Receive(uint16* rx, stduint size, IOMethod method) {
		if (!rx || !size) return 0;
		const bool is32 = (datafmt == I2SDataFormat_E::Bits24) || (datafmt == I2SDataFormat_E::Bits32);
		volatile uint32* rxdr = (volatile uint32*)(i2s_addr[id - 1] + _IMM(SPIReg::RXDR));
		switch (method) {
		case IOMethod::Rupt: {
			// AKA HAL_I2S_Receive_IT
			if (state != I2SXState::Ready) return 0;
			state = I2SXState::BusyRX; error_code = _I2S_ERR_NONE;
			rx_buff = rx; rx_size = size; rx_count = size;
			tx_buff = nullptr; tx_size = 0; tx_count = 0;
			setDirection(1);
			enAble(true);
		#if defined(_MCU_STM32H7x)
			if (mode == I2SMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		#elif defined(_MPU_STM32MP13)
			self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		#endif
			self[SPIReg::IER] = SPI_IER_RXPIE | SPI_IER_UDRIE | SPI_IER_OVRIE | SPI_IER_TIFREIE;
			return size;
		}
		case IOMethod::DMA: {
			// AKA HAL_I2S_Receive_DMA
			if (state != I2SXState::Ready) return 0;
			state = I2SXState::BusyRX; error_code = _I2S_ERR_NONE;
			rx_buff = rx; rx_size = size; rx_count = size;
			tx_buff = nullptr; tx_size = 0; tx_count = 0;
			setDirection(1);
			if (mode == I2SMode_E::Master) {
				// AKA H7/MP13: clear overrun before Master RX
				self[SPIReg::IFCR] = SPI_IFCR_OVRC;
			}
			self[SPIReg::CFG1].setof(SPI_CFG1_RXDMAEN_Pos);
			enAble(true);
		#if defined(_MCU_STM32H7x)
			if (mode == I2SMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		#elif defined(_MPU_STM32MP13)
			self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		#endif
			if (!dmaRxStart((pureptr_t)rxdr, (pureptr_t)rx, size)) {
				self[SPIReg::CFG1].rstof(SPI_CFG1_RXDMAEN_Pos);
				state = I2SXState::Ready; return 0;
			}
			return size;
		}
		case IOMethod::Loop: break;
		}
		// Loop (AKA HAL_I2S_Receive)
		if (state != I2SXState::Ready) return 0;
		state = I2SXState::BusyRX; error_code = _I2S_ERR_NONE;
		rx_buff = rx; rx_size = size; rx_count = size;
		tx_buff = nullptr; tx_size = 0; tx_count = 0;
		setDirection(1);
		enAble(true);
	#if defined(_MCU_STM32H7x)
		if (mode == I2SMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		// AKA H7: clear overrun in Master RX mode
		if (mode == I2SMode_E::Master) self[SPIReg::IFCR] = SPI_IFCR_OVRC;
	#elif defined(_MPU_STM32MP13)
		self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
	#endif
		uint64 tickstart = SysTick::getTick();
		while (rx_count > 0) {
			if (self[SPIReg::SR].bitof(SPI_SR_RXP_Pos)) {
				if (is32) {
					*(uint32*)rx_buff = *rxdr; rx_buff += 2; rx_count--;
				}
			#if defined(_MCU_STM32H7x)
				// AKA H7: RXWNE -> 32-bit read (2 frames)
				else if (self[SPIReg::SR].bitof(SPI_SR_RXWNE_Pos)) {
					*(uint32*)rx_buff = *rxdr; rx_buff += 2; rx_count -= 2;
				}
			#endif
				else {
					*(uint16*)rx_buff = *(volatile uint16*)rxdr; rx_buff += 1; rx_count--;
				}
			}
			else if ((SysTick::getTick() - tickstart) > _I2S_TIMEOUT_VALUE) {
				error_code |= _I2S_ERR_TIMEOUT; break;
			}
		#if defined(_MPU_STM32MP13)
			if (self[SPIReg::SR].bitof(SPI_SR_OVR_Pos)) {
				self[SPIReg::IFCR] = SPI_IFCR_OVRC;
				error_code |= _I2S_ERR_OVR;
			}
		#endif
		}
		state = I2SXState::Ready;
		return (error_code & _I2S_ERR_TIMEOUT) ? 0 : size;
	}

	// AKA HAL_I2SEx_TransmitReceive / _IT / _DMA (full-duplex)
	stduint I2S_HARD::Transceive(const uint16* tx, uint16* rx, stduint size, IOMethod method) {
		if (!tx || !rx || !size) return 0;
		const bool is32 = (datafmt == I2SDataFormat_E::Bits24) || (datafmt == I2SDataFormat_E::Bits32);
		volatile uint32* txdr = (volatile uint32*)(i2s_addr[id - 1] + _IMM(SPIReg::TXDR));
		volatile uint32* rxdr = (volatile uint32*)(i2s_addr[id - 1] + _IMM(SPIReg::RXDR));
		switch (method) {
		case IOMethod::Rupt: {
			// AKA HAL_I2SEx_TransmitReceive_IT
			if (state != I2SXState::Ready) return 0;
			state = I2SXState::BusyTxRx; error_code = _I2S_ERR_NONE;
			tx_buff = tx; tx_size = size; tx_count = size;
			rx_buff = rx; rx_size = size; rx_count = size;
			setDirection(2);
			enAble(true);
		#if defined(_MCU_STM32H7x)
			if (mode == I2SMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		#elif defined(_MPU_STM32MP13)
			self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		#endif
			self[SPIReg::IER] = SPI_IER_TXPIE | SPI_IER_RXPIE | SPI_IER_UDRIE | SPI_IER_OVRIE | SPI_IER_TIFREIE;
			return size;
		}
		case IOMethod::DMA: {
			// AKA HAL_I2SEx_TransmitReceive_DMA
			if (state != I2SXState::Ready) return 0;
			state = I2SXState::BusyTxRx; error_code = _I2S_ERR_NONE;
			tx_buff = tx; tx_size = size; tx_count = size;
			rx_buff = rx; rx_size = size; rx_count = size;
			setDirection(2);
			self[SPIReg::CFG1].setof(SPI_CFG1_TXDMAEN_Pos);
			if (!dmaTxStart((pureptr_t)tx, (pureptr_t)txdr, size)) {
				self[SPIReg::CFG1].rstof(SPI_CFG1_TXDMAEN_Pos);
				state = I2SXState::Ready; return 0;
			}
			self[SPIReg::CFG1].setof(SPI_CFG1_RXDMAEN_Pos);
			if (!dmaRxStart((pureptr_t)rxdr, (pureptr_t)rx, size)) {
				self[SPIReg::CFG1].rstof(SPI_CFG1_RXDMAEN_Pos);
				state = I2SXState::Ready; return 0;
			}
			enAble(true);
		#if defined(_MCU_STM32H7x)
			if (mode == I2SMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		#elif defined(_MPU_STM32MP13)
			self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		#endif
			return size;
		}
		case IOMethod::Loop: break;
		}
		// Loop (AKA HAL_I2SEx_TransmitReceive)
		if (state != I2SXState::Ready) return 0;
		state = I2SXState::BusyTxRx; error_code = _I2S_ERR_NONE;
		tx_buff = tx; tx_size = size; tx_count = size;
		rx_buff = rx; rx_size = size; rx_count = size;
		setDirection(2);
		enAble(true);
	#if defined(_MCU_STM32H7x)
		if (mode == I2SMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
	#elif defined(_MPU_STM32MP13)
		self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
	#endif
		uint64 tickstart = SysTick::getTick();
		while (tx_count > 0 || rx_count > 0) {
			if (tx_count > 0 && self[SPIReg::SR].bitof(SPI_SR_TXP_Pos)) {
				if (is32) {
					*txdr = *(const uint32*)tx_buff; tx_buff += 2; tx_count--;
				}
			#if defined(_MCU_STM32H7x)
				else if (tx_count > 1 && _IMM(fifo_thr) > 0x00) {
					*txdr = *(const uint32*)tx_buff; tx_buff += 2; tx_count -= 2;
				}
			#endif
				else {
					*(volatile uint16*)txdr = *tx_buff; tx_buff += 1; tx_count--;
				}
			}
			if (rx_count > 0 && self[SPIReg::SR].bitof(SPI_SR_RXP_Pos)) {
				if (is32) {
					*(uint32*)rx_buff = *rxdr; rx_buff += 2; rx_count--;
				}
			#if defined(_MCU_STM32H7x)
				else if (self[SPIReg::SR].bitof(SPI_SR_RXWNE_Pos)) {
					*(uint32*)rx_buff = *rxdr; rx_buff += 2; rx_count -= 2;
				}
			#endif
				else {
					*(uint16*)rx_buff = *(volatile uint16*)rxdr; rx_buff += 1; rx_count--;
				}
			}
			if ((SysTick::getTick() - tickstart) > _I2S_TIMEOUT_VALUE) {
				error_code |= _I2S_ERR_TIMEOUT; break;
			}
		}
	#if defined(_MCU_STM32H7x)
		if (!waitFlag(SPI_SR_TXP_Pos, false, tickstart, _I2S_TIMEOUT_VALUE)) error_code |= _I2S_ERR_TIMEOUT;
	#endif
		state = I2SXState::Ready;
		return (error_code & _I2S_ERR_TIMEOUT) ? 0 : size;
	}

	// AKA HAL_I2S_DMAPause
	// H7: disable TX/RX DMA requests (peripheral keeps running; DMA stops feeding)
	// MP13: master-only hardware suspend (CSUSP -> wait CSTART clear -> disable)
	bool I2S_HARD::PauseDMA() {
	#if defined(_MCU_STM32H7x)
		bool was_on = self[SPIReg::CR1].bitof(SPI_CR1_SPE_Pos);
		if (was_on) enAble(false);
		self[SPIReg::CFG1] &= ~(SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);
		if (was_on) enAble(true);
		return true;
	#elif defined(_MPU_STM32MP13)
		if (mode != I2SMode_E::Master) { setError(_I2S_ERR_NOT_SUPPORTED); state = I2SXState::Ready; return false; }
		if (!self[SPIReg::CR1].bitof(SPI_CR1_CSTART_Pos)) { setError(_I2S_ERR_NO_OGT); state = I2SXState::Ready; return false; }
		self[SPIReg::CR1].setof(SPI_CR1_CSUSP_Pos);
		uint64 tickstart = SysTick::getTick();
		while (self[SPIReg::CR1].bitof(SPI_CR1_CSTART_Pos)) {
			if ((SysTick::getTick() - tickstart) > 0xFFFF) {
				setError(_I2S_ERR_TIMEOUT);
				state = I2SXState::Ready;
				return false;
			}
		}
		enAble(false);
		state = I2SXState::Ready;
		return true;
	#endif
	}

	// AKA HAL_I2S_DMAResume
	// H7: re-enable TX/RX DMA requests and ensure peripheral enabled
	// MP13: re-enable peripheral and restart the transfer (CSTART)
	bool I2S_HARD::ResumeDMA() {
	#if defined(_MCU_STM32H7x)
		bool was_on = self[SPIReg::CR1].bitof(SPI_CR1_SPE_Pos);
		if (was_on) enAble(false);
		self[SPIReg::CFG1] |= (SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);
		enAble(true);
		return true;
	#elif defined(_MPU_STM32MP13)
		if (state != I2SXState::Ready) { state = I2SXState::Ready; return false; }
		error_code = _I2S_ERR_NONE;
		// restore the direction state from the still-enabled DMA request bit
		state = self[SPIReg::CFG1].bitof(SPI_CFG1_TXDMAEN_Pos) ? I2SXState::BusyTX : I2SXState::BusyRX;
		enAble(true);
		self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		return true;
	#endif
	}

	// AKA HAL_I2S_DMAStop: clear DMA requests, abort DMA channels, disable I2S
	bool I2S_HARD::StopDMA() {
		self[SPIReg::CFG1] &= ~(SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);
		bool ok = true;
		if (hdmatx && !hdmatx->Abort()) { setError(_I2S_ERR_DMA); ok = false; }
		if (hdmarx && !hdmarx->Abort()) { setError(_I2S_ERR_DMA); ok = false; }
		enAble(false);
		state = I2SXState::Ready;
		return ok;
	}

	// AKA I2S_WaitFlagStateUntilTimeout
	bool I2S_HARD::waitFlag(stduint pos, bool val, uint64 tickstart, stduint timeout) {
		while (self[SPIReg::SR].bitof(pos) != val) {
			if (timeout != ~0u && (SysTick::getTick() - tickstart) > timeout)
				return false;
		}
		return true;
	}

	// AKA I2S_DMATxCplt / I2S_DMARxCplt / I2S_DMAError dispatch (DMA layer callbacks)
	static void i2s_dmaCplt() {
		I2S_HARD* sp = nullptr;
		if (DMA1.XferCpltCallback == i2s_dmaCplt) sp = (I2S_HARD*)DMA1.bind;
		else if (DMA2.XferCpltCallback == i2s_dmaCplt) sp = (I2S_HARD*)DMA2.bind;
		if (!sp) return;
		sp->dmaCpltISR();
	}
	static void i2s_dmaError() {
		I2S_HARD* sp = nullptr;
		if (DMA1.XferErrorCallback == i2s_dmaError) sp = (I2S_HARD*)DMA1.bind;
		else if (DMA2.XferErrorCallback == i2s_dmaError) sp = (I2S_HARD*)DMA2.bind;
		if (!sp) return;
		sp->setError(_I2S_ERR_DMA);
		sp->dmaCpltISR();
	}

	// TX DMA start: memory -> peripheral (TXDR)
	bool I2S_HARD::dmaTxStart(pureptr_t mem, pureptr_t peri, stduint ndtr) {
		if (!hdmatx) return false;
		DMA_t& dma = hdmatx->getParent();
		dma.bind = (pureptr_t)this;
		dma.XferCpltCallback = i2s_dmaCplt;
		dma.XferErrorCallback = i2s_dmaError;
		return hdmatx->Transfer(mem, peri, ndtr, IOMethod::Rupt);
	}
	// RX DMA start: peripheral (RXDR) -> memory
	bool I2S_HARD::dmaRxStart(pureptr_t peri, pureptr_t mem, stduint ndtr) {
		if (!hdmarx) return false;
		DMA_t& dma = hdmarx->getParent();
		dma.bind = (pureptr_t)this;
		dma.XferCpltCallback = i2s_dmaCplt;
		dma.XferErrorCallback = i2s_dmaError;
		return hdmarx->Transfer(peri, mem, ndtr, IOMethod::Rupt);
	}

	// AKA I2S_DMATxCplt / I2S_DMARxCplt: DMA finished, close transfer
	void I2S_HARD::dmaCpltISR() {
		I2SXState st = state;
		self[SPIReg::CFG1] &= ~(SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);
		tx_count = 0; rx_count = 0;
		state = I2SXState::Ready;
		if (error_code != _I2S_ERR_NONE) { if (ErrorHandler) ErrorHandler(); return; }
		if (st == I2SXState::BusyTxRx) { if (TxRxCpltHandler) TxRxCpltHandler(); }
		else if (st == I2SXState::BusyTX) { if (TxCpltHandler) TxCpltHandler(); }
		else { if (RxCpltHandler) RxCpltHandler(); }
	}

	// AKA HAL_I2S_IRQHandler (Receiver branch) + I2S_RxISR_16BIT/32BIT
	void I2S_HARD::innHandlerByInterrupt() {
		const bool is32 = (datafmt == I2SDataFormat_E::Bits24) || (datafmt == I2SDataFormat_E::Bits32);
		volatile uint32* rxdr = (volatile uint32*)(i2s_addr[id - 1] + _IMM(SPIReg::RXDR));
		// RX data (skip if overrun occurred, AKA HAL: !OVR && RXNE && RXPIE)
		if (self[SPIReg::SR].bitof(SPI_SR_RXP_Pos) && !self[SPIReg::SR].bitof(SPI_SR_OVR_Pos)) {
			if (is32) { *(uint32*)rx_buff = *rxdr; rx_buff += 2; rx_count--; }
			else { *(uint16*)rx_buff = *(volatile uint16*)rxdr; rx_buff += 1; rx_count--; }
			if (rx_count == 0) closeRxISR();
		}
		// OVR error (AKA HAL: disable RXNE|ERR, clear OVR, Ready, ErrorCallback)
		if (self[SPIReg::SR].bitof(SPI_SR_OVR_Pos)) {
			self[SPIReg::IER] &= ~(SPI_IER_RXPIE | SPI_IER_UDRIE | SPI_IER_OVRIE | SPI_IER_TIFREIE);
			self[SPIReg::IFCR] = SPI_IFCR_OVRC;
			error_code |= _I2S_ERR_OVR;
			state = I2SXState::Ready;
			if (ErrorHandler) ErrorHandler();
		}
	}

	// AKA HAL_I2S_IRQHandler (Transmitter branch) + I2S_TxISR_16BIT/32BIT
	void I2S_HARD::outHandlerByInterrupt() {
		const bool is32 = (datafmt == I2SDataFormat_E::Bits24) || (datafmt == I2SDataFormat_E::Bits32);
		volatile uint32* txdr = (volatile uint32*)(i2s_addr[id - 1] + _IMM(SPIReg::TXDR));
		// TX data (skip if underrun occurred, AKA HAL: !UDR && TXE && TXPIE)
		if (self[SPIReg::SR].bitof(SPI_SR_TXP_Pos) && !self[SPIReg::SR].bitof(SPI_SR_UDR_Pos)) {
			if (is32) { *txdr = *(const uint32*)tx_buff; tx_buff += 2; tx_count--; }
			else { *(volatile uint16*)txdr = *tx_buff; tx_buff += 1; tx_count--; }
			if (tx_count == 0) closeTxISR();
		}
		// UDR error (AKA HAL: disable TXE|ERR, clear UDR, Ready, ErrorCallback)
		if (self[SPIReg::SR].bitof(SPI_SR_UDR_Pos)) {
			self[SPIReg::IER] &= ~(SPI_IER_TXPIE | SPI_IER_UDRIE | SPI_IER_OVRIE | SPI_IER_TIFREIE);
			self[SPIReg::IFCR] = SPI_IFCR_UDRC;
			error_code |= _I2S_ERR_UDR;
			state = I2SXState::Ready;
			if (ErrorHandler) ErrorHandler();
		}
	}

	// AKA I2S_CloseRx_ISR
	void I2S_HARD::closeRxISR() {
		self[SPIReg::IER] &= ~(SPI_IER_RXPIE | SPI_IER_UDRIE | SPI_IER_OVRIE | SPI_IER_TIFREIE);
		// AKA H7: in FD mode write dummy TX to avoid underrun, then clear UDR
		if (i2scfg_cur == 4 || i2scfg_cur == 5) {
			*(volatile uint8*)(i2s_addr[id - 1] + _IMM(SPIReg::TXDR)) = 0x01;
			self[SPIReg::IFCR] = SPI_IFCR_UDRC;
		}
		state = I2SXState::Ready;
		if (error_code != _I2S_ERR_NONE) { if (ErrorHandler) ErrorHandler(); }
		else { if (RxCpltHandler) RxCpltHandler(); }
	}

	// AKA I2S_CloseTx_ISR
	void I2S_HARD::closeTxISR() {
		self[SPIReg::IER] &= ~(SPI_IER_TXPIE | SPI_IER_UDRIE | SPI_IER_OVRIE | SPI_IER_TIFREIE);
		// AKA H7: in FD mode clear overrun (received data not read)
		if (i2scfg_cur == 4 || i2scfg_cur == 5) {
			self[SPIReg::IFCR] = SPI_IFCR_OVRC;
		}
		state = I2SXState::Ready;
		if (error_code != _I2S_ERR_NONE) { if (ErrorHandler) ErrorHandler(); }
		else { if (TxCpltHandler) TxCpltHandler(); }
	}

	I2S_HARD I2S1(1), I2S2(2), I2S3(3);

#endif

}
