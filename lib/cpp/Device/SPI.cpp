// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Protocol) Serial Peripheral interface, SPI
// Codifiers: @dosconio: 20240730~;
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: GPIO
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


#include "../../../inc/c/driver/SPI.h"
#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
#include "../../../inc/cpp/Device/SysTick"
#include "../../../inc/cpp/Device/DMA"
#endif
#if defined(_MCU_STM32H7x)
#include "../../../inc/cpp/Device/BDMA"
#endif

namespace uni {
#if defined(_SUPPORT_GPIO)

	byte SPI_SOFT::Transceive(byte data) {
		byte res = 0;// rx
		for0(i, _BYTE_BITS_) {
			SCLK = 0;
			asserv(func_delay)();
			MOSI = data & 0x80;
			data <<= 1;
			asserv(func_delay)();
			SCLK.Toggle();
			asserv(func_delay)();
			res <<= 1;
			if (&MISO != &MOSI && MISO) res |= 1;
		}
		SCLK = 0;
		return res;
	}

	SPI_SOFT::SPI_SOFT(GPIO_Pin& SCLK, GPIO_Pin& MOSI, GPIO_Pin& MISO, GPIO_Pin& CSEL) : SCLK(SCLK), MOSI(MOSI), MISO(MISO), CSEL(CSEL)
	{
		SCLK.setMode(GPIOMode::OUT_PushPull);
		if (&MISO != &MOSI)
			MISO.setMode(GPIOMode::IN_Floating);
		MOSI.setMode(GPIOMode::OUT_PushPull);
		CSEL.setMode(GPIOMode::OUT_PushPull);
		CSEL = 1;
		SCLK = 0;
		MOSI = _TEMP 0;
	}

	stduint SPI_SOFT::Transceivex(stduint data, byte blen) {
		stduint res = 0;// rx
		MIN(blen, bitsof(stduint));
		stduint mask = 1 << (blen - 1);
		for0(i, blen) {
			SCLK = 0;
			asserv(func_delay)();
			MOSI = data & mask;// DIN
			data <<= 1;
			asserv(func_delay)();
			SCLK.Toggle();
			asserv(func_delay)();
			res <<= 1;
			if (&MISO != &MOSI && MISO) res |= 1;
		}
		SCLK = 0;
		return res;
	}

	void SPI_SOFT::SendStart() {
		CSEL = 1;
		SCLK = 0;
		MOSI = 0;
		CSEL.Toggle();
	}

	void SPI_SOFT::SendStop() {
		MOSI = 0;
		CSEL = 1;
	}


#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)

#if defined(_MCU_STM32H7x)
	static const stduint spin_addr[6] = {
		D2_APB2PERIPH_BASE + 0x3000,// SPI1
		D2_APB1PERIPH_BASE + 0x3800,// SPI2
		D2_APB1PERIPH_BASE + 0x3C00,// SPI3
		D2_APB2PERIPH_BASE + 0x3400,// SPI4
		D2_APB2PERIPH_BASE + 0x5000,// SPI5
		D3_APB1PERIPH_BASE + 0x1400,// SPI6
	};
#elif defined(_MPU_STM32MP13)
	static const stduint spin_addr[5] = {
		APB2_PERIPH_BASE + 0x4000,// SPI1
		APB1_PERIPH_BASE + 0xB000,// SPI2
		APB1_PERIPH_BASE + 0xC000,// SPI3
		APB6_PERIPH_BASE + 0x2000,// SPI4
		APB6_PERIPH_BASE + 0x3000,// SPI5
	};
#endif

	Reference SPI_HARD::operator[](SPIReg reg) {
		return spin_addr[id - 1] + _IMM(reg);
	}

	bool SPI_HARD::enClock(bool ena) {
	#if defined(_MCU_STM32H7x)
		asrtret(Ranglin(id, 1, 6));
		auto reg = RCCReg::APB1LENR; byte bit = 0;
		switch (id) {
		case 1: reg = RCCReg::APB2ENR; bit = 12; break;
		case 2: reg = RCCReg::APB1LENR; bit = 14; break;
		case 3: reg = RCCReg::APB1LENR; bit = 15; break;
		case 4: reg = RCCReg::APB2ENR; bit = 13; break;
		case 5: reg = RCCReg::APB2ENR; bit = 20; break;
		case 6: reg = RCCReg::APB4ENR; bit = 5; break;
		default: return false;
		}
		RCC[reg].setof(bit, ena);
		return RCC[reg].bitof(bit) == ena;
	#elif defined(_MPU_STM32MP13)
		asrtret(Ranglin(id, 1, 5));
		auto reg = RCCReg::MP_APB1ENSETR; byte bit = 0;
		switch (id) {
		case 1: reg = ena ? RCCReg::MP_APB2ENSETR : RCCReg::MP_APB2ENCLRR; bit = 8; break;
		case 2: reg = ena ? RCCReg::MP_APB1ENSETR : RCCReg::MP_APB1ENCLRR; bit = 11; break;
		case 3: reg = ena ? RCCReg::MP_APB1ENSETR : RCCReg::MP_APB1ENCLRR; bit = 12; break;
		case 4: reg = ena ? RCCReg::MP_APB6ENSETR : RCCReg::MP_APB6ENCLRR; bit = 2; break;
		case 5: reg = ena ? RCCReg::MP_APB6ENSETR : RCCReg::MP_APB6ENCLRR; bit = 3; break;
		default: return false;
		}
		RCC[reg] = _IMM1S(bit);
		return true;
	#endif
	}

	bool SPI_HARD::enAble(bool ena) {
		self[SPIReg::CR1].setof(SPI_CR1_SPE_Pos, ena);
		return true;
	}

	bool SPI_HARD::canMode() {
		if (!enClock()) return false;
		enAble(false);

		// AKA SPI_Init: soft NSS + master + low polarity => internal SSI = 1
		if (nss == SPINss_E::Soft && mode == SPIMode_E::Master && nss_polarity == SPINssPolarity_E::Low)
			self[SPIReg::CR1].setof(SPI_CR1_SSI_Pos);

		// AKA SPI_Init: align CRC length on datasize
		stduint crc_length_val = 0;
		if (crc_calc == SPICrc_E::Enable) {
			if (crc_length == SPICrcLength_E::DataSize)
				crc_length_val = _IMM(datasize) << SPI_CFG1_CRCSIZE_Pos;
			else
				crc_length_val = _IMM(crc_length);
		}

		// AKA SPI_Init: CFG1 = baud | crc | crc_length | fifo | datasize
		self[SPIReg::CFG1] = _IMM(baudrate) | _IMM(crc_calc) | crc_length_val | _IMM(fifo_threshold) | _IMM(datasize);
		// AKA SPI_Init: CFG2 = nss_pulse | ti | nss_polarity | nss | cpol | cpha | first_bit | mode | midi | direction | mssi | ioswap
		self[SPIReg::CFG2] = _IMM(nss_pulse) | _IMM(ti_mode) | _IMM(nss_polarity) | _IMM(nss)
			| _IMM(clk_polarity) | _IMM(clk_phase) | _IMM(first_bit) | _IMM(mode)
			| _IMM(interdata_idle) | _IMM(direction) | _IMM(ss_idle) | _IMM(ioswap);

		if (crc_calc == SPICrc_E::Enable) {
			self[SPIReg::CR1].setof(SPI_CR1_TCRCINI_Pos, crc_tx_init == SPICrcInitPattern_E::AllOne);
			self[SPIReg::CR1].setof(SPI_CR1_RCRCINI_Pos, crc_rx_init == SPICrcInitPattern_E::AllOne);
			self[SPIReg::CR1].setof(SPI_CR1_CRC33_17_Pos, crc_length_val == _IMM(SPICrcLength_E::Bits32));
			self[SPIReg::CRCPOLY] = crc_polynomial;
		}

		// AKA SPI_Init: slave underrun default
		if (mode == SPIMode_E::Slave) {
			self[SPIReg::CFG1].maset(SPI_CFG1_UDRDET_Pos, 2, 1);// UDRDET_0
			self[SPIReg::CFG1].maset(SPI_CFG1_UDRCFG_Pos, 2, 2);// UDRCFG_1
		}

		// AKA SPI_Init: master AFCNTR
		if (mode == SPIMode_E::Master)
			self[SPIReg::CFG2].setof(SPI_CFG2_AFCNTR_Pos, keep_io == SPIMasterKeepIO_E::Enable);

		// AKA SPI_Init: ensure SPI mode (I2SMOD reset)
		self[SPIReg::I2SCFGR].rstof(SPI_I2SCFGR_I2SMOD_Pos);

		// AKA SPI_Init: master RX autosuspend
		if (mode == SPIMode_E::Master)
			self[SPIReg::CR1].setof(SPI_CR1_MASRX_Pos, rx_autosusp == SPIMasterRxAutoSusp_E::Enable);

		return true;
	}

	byte SPI_HARD::Transceive(byte data) {
		self[SPIReg::CR2].maset(SPI_CR2_TSIZE_Pos, 16, 1);
		enAble(true);
		if (mode == SPIMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		while (!self[SPIReg::SR].bitof(SPI_SR_TXP_Pos));
		self[SPIReg::TXDR] = data;
		while (!self[SPIReg::SR].bitof(SPI_SR_RXWNE_Pos));
		byte res = (byte)self[SPIReg::RXDR];
		while (!self[SPIReg::SR].bitof(SPI_SR_EOT_Pos));
		self[SPIReg::IFCR] = SPI_IFCR_EOTC;
		enAble(false);
		return res;
	}

	stduint SPI_HARD::Transceivex(stduint data, byte blen) {
		MIN(blen, 32);
		if (blen < 4) blen = 4;
		stduint old_cfg1 = self[SPIReg::CFG1];
		self[SPIReg::CFG1].maset(SPI_CFG1_DSIZE_Pos, 5, blen - 1);
		self[SPIReg::CR2].maset(SPI_CR2_TSIZE_Pos, 16, 1);
		enAble(true);
		if (mode == SPIMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		while (!self[SPIReg::SR].bitof(SPI_SR_TXP_Pos));
		self[SPIReg::TXDR] = data;
		while (!self[SPIReg::SR].bitof(SPI_SR_RXWNE_Pos));
		stduint res = self[SPIReg::RXDR];
		while (!self[SPIReg::SR].bitof(SPI_SR_EOT_Pos));
		self[SPIReg::IFCR] = SPI_IFCR_EOTC;
		enAble(false);
		self[SPIReg::CFG1] = old_cfg1;
		return res;
	}

	void SPI_HARD::SendStart() {
		if (nss == SPINss_E::Soft) self[SPIReg::CR1].setof(SPI_CR1_SSI_Pos, false);
	}
	void SPI_HARD::SendStop() {
		if (nss == SPINss_E::Soft) self[SPIReg::CR1].setof(SPI_CR1_SSI_Pos, true);
	}

	// AKA SPI_CloseTransfer: disable + clear flags + report errors
	void SPI_HARD::closeTransfer() {
		stduint itflag = self[SPIReg::SR];
		self[SPIReg::IFCR] = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;
		enAble(false);
		self[SPIReg::IER] = 0;
		self[SPIReg::CFG1] &= ~(SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);
		if (state != SPIXState::BusyRX && (itflag & SPI_SR_UDR)) {
			error_code |= _SPI_ERR_UDR;
			self[SPIReg::IFCR] = SPI_IFCR_UDRC;
		}
		if (state != SPIXState::BusyTX && (itflag & SPI_SR_OVR)) {
			error_code |= _SPI_ERR_OVR;
			self[SPIReg::IFCR] = SPI_IFCR_OVRC;
		}
		if (itflag & SPI_SR_MODF) {
			error_code |= _SPI_ERR_MODF;
			self[SPIReg::IFCR] = SPI_IFCR_MODFC;
		}
		if (itflag & SPI_SR_TIFRE) {
			error_code |= _SPI_ERR_FRE;
			self[SPIReg::IFCR] = SPI_IFCR_TIFREC;
		}
		tx_count = 0; rx_count = 0;
	}

	// AKA SPI_WaitOnFlagUntilTimeout
	bool SPI_HARD::waitFlag(stduint pos, bool val, uint64 tickstart, stduint timeout) {
		while (self[SPIReg::SR].bitof(pos) != val) {
			if (timeout != ~0u && (SysTick::getTick() - tickstart) > timeout)
				return false;
		}
		return true;
	}

	// H7/MP13 DMAMUX1 request line IDs for SPI1~SPI5 (SPI6 routes via BDMA/DMAMUX2)
	static stduint spi_dmaRequestID(byte spid, bool is_tx) {
		static const stduint req_tbl[2][5] = {
			{ 37, 39, 61, 83, 85 }, // RX
			{ 38, 40, 62, 84, 86 }  // TX
		};
		if (!Ranglin(spid, 1, 5)) return 0;
		return req_tbl[is_tx ? 1 : 0][spid - 1];
	}

	// AKA SPI_DMATransmitCplt / SPI_DMAReceiveCplt / SPI_DMATransmitReceiveCplt
	static void spi_dmaCplt() {
		SPI_HARD* sp = nullptr;
		if (DMA1.XferCpltCallback == spi_dmaCplt) sp = (SPI_HARD*)DMA1.bind;
		else if (DMA2.XferCpltCallback == spi_dmaCplt) sp = (SPI_HARD*)DMA2.bind;
	#if defined(_MCU_STM32H7x)
		else if (BDMA.XferCpltCallback == spi_dmaCplt) sp = (SPI_HARD*)BDMA.bind;
	#endif
		if (!sp) return;
		// DMA has filled/flushed the buffer; EOT interrupt will finish the transfer
	}
	static void spi_dmaError() {
		SPI_HARD* sp = nullptr;
		if (DMA1.XferErrorCallback == spi_dmaError) sp = (SPI_HARD*)DMA1.bind;
		else if (DMA2.XferErrorCallback == spi_dmaError) sp = (SPI_HARD*)DMA2.bind;
	#if defined(_MCU_STM32H7x)
		else if (BDMA.XferErrorCallback == spi_dmaError) sp = (SPI_HARD*)BDMA.bind;
	#endif
		if (!sp) return;
		sp->setError(_SPI_ERR_DMA);
		sp->closeTransfer();
		sp->setState(SPIXState::Ready);
		if (sp->ErrorHandler) sp->ErrorHandler();
	}

	// TX DMA start: memory -> peripheral (TXDR)
	bool SPI_HARD::dmaTxStart(pureptr_t mem, pureptr_t peri, stduint ndtr) {
	#if defined(_MCU_STM32H7x)
		if (id == 6) {
			if (bdma_tx_channel > 7) return false;
			BDMA.bind = (pureptr_t)this;
			BDMA.XferCpltCallback = spi_dmaCplt;
			BDMA.XferErrorCallback = spi_dmaError;
			BDMA.setRequest(bdma_tx_channel, 12);// BDMA_REQUEST_SPI6_TX (DMAMUX2)
			return BDMA.Transfer(bdma_tx_channel, mem, peri, ndtr, IOMethod::Rupt);
		}
	#endif
		if (!hdmatx) return false;
		DMA_t& dma = hdmatx->getParent();
		dma.bind = (pureptr_t)this;
		dma.XferCpltCallback = spi_dmaCplt;
		dma.XferErrorCallback = spi_dmaError;
		return hdmatx->Transfer(mem, peri, ndtr, IOMethod::Rupt);
	}
	// RX DMA start: peripheral (RXDR) -> memory
	bool SPI_HARD::dmaRxStart(pureptr_t peri, pureptr_t mem, stduint ndtr) {
	#if defined(_MCU_STM32H7x)
		if (id == 6) {
			if (bdma_rx_channel > 7) return false;
			BDMA.bind = (pureptr_t)this;
			BDMA.XferCpltCallback = spi_dmaCplt;
			BDMA.XferErrorCallback = spi_dmaError;
			BDMA.setRequest(bdma_rx_channel, 11);// BDMA_REQUEST_SPI6_RX (DMAMUX2)
			return BDMA.Transfer(bdma_rx_channel, peri, mem, ndtr, IOMethod::Rupt);
		}
	#endif
		if (!hdmarx) return false;
		DMA_t& dma = hdmarx->getParent();
		dma.bind = (pureptr_t)this;
		dma.XferCpltCallback = spi_dmaCplt;
		dma.XferErrorCallback = spi_dmaError;
		return hdmarx->Transfer(peri, mem, ndtr, IOMethod::Rupt);
	}

	// AKA HAL_SPI_Transmit
	stduint SPI_HARD::Transmit(const byte* tx, stduint size, IOMethod method) {
		if (!tx || !size) return 0;
		const stduint ds = _IMM(datasize);
		volatile uint32* txdr = (volatile uint32*)(spin_addr[id - 1] + _IMM(SPIReg::TXDR));
		switch (method) {
		case IOMethod::Rupt: {
			// AKA HAL_SPI_Transmit_IT
			if (state != SPIXState::Ready) return 0;
			state = SPIXState::BusyTX; error_code = 0;
			tx_buff = tx; tx_size = size; tx_count = size;
			rx_buff = nullptr; rx_size = 0; rx_count = 0;
			self[SPIReg::CR2].maset(SPI_CR2_TSIZE_Pos, 16, size);
			enAble(true);
			self[SPIReg::IER] = SPI_IER_EOTIE | SPI_IER_TXPIE | SPI_IER_UDRIE | SPI_IER_TIFREIE | SPI_IER_MODFIE;
			if (mode == SPIMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
			return size;
		}
		case IOMethod::DMA: {
			// AKA HAL_SPI_Transmit_DMA
			if (state != SPIXState::Ready) return 0;
			state = SPIXState::BusyTX; error_code = 0;
			tx_buff = tx; tx_size = size; tx_count = size;
			rx_buff = nullptr; rx_size = 0; rx_count = 0;
			self[SPIReg::CR2].maset(SPI_CR2_TSIZE_Pos, 16, size);
			stduint ndtr = size / (ds > 15 ? 4 : (ds > 7 ? 2 : 1));
			self[SPIReg::CFG1].setof(SPI_CFG1_TXDMAEN_Pos);
			enAble(true);
			self[SPIReg::IER] = SPI_IER_EOTIE | SPI_IER_UDRIE | SPI_IER_TIFREIE | SPI_IER_MODFIE;
			if (mode == SPIMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
			if (!dmaTxStart((pureptr_t)tx, (pureptr_t)txdr, ndtr)) {
				self[SPIReg::CFG1].rstof(SPI_CFG1_TXDMAEN_Pos);
				state = SPIXState::Ready; return 0;
			}
			return size;
		}
		case IOMethod::Loop: break;
		}
		// Loop (AKA HAL_SPI_Transmit)
		if (state != SPIXState::Ready) return 0;
		state = SPIXState::BusyTX; error_code = 0;
		tx_buff = tx; tx_size = size; tx_count = size;
		rx_buff = nullptr; rx_size = 0; rx_count = 0;
		self[SPIReg::CR2].maset(SPI_CR2_TSIZE_Pos, 16, size);
		enAble(true);
		if (mode == SPIMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		uint64 tickstart = SysTick::getTick();
		while (tx_count > 0) {
			if (self[SPIReg::SR].bitof(SPI_SR_TXP_Pos)) {
				if (ds > 15) {
					*txdr = *(const uint32*)tx_buff; tx_buff += 4; tx_count--;
				}
				else if (ds > 7) {
					if (tx_count > 1 && _IMM(fifo_threshold) > 0x00) {
						*txdr = *(const uint32*)tx_buff; tx_buff += 4; tx_count -= 2;
					} else {
						*(volatile uint16*)txdr = *(const uint16*)tx_buff; tx_buff += 2; tx_count--;
					}
				}
				else {
					if (tx_count > 3 && _IMM(fifo_threshold) > 0x40) {
						*txdr = *(const uint32*)tx_buff; tx_buff += 4; tx_count -= 4;
					} else if (tx_count > 1 && _IMM(fifo_threshold) > 0x00) {
						*(volatile uint16*)txdr = *(const uint16*)tx_buff; tx_buff += 2; tx_count -= 2;
					} else {
						*(volatile uint8*)txdr = *tx_buff; tx_buff += 1; tx_count--;
					}
				}
			}
			else if ((SysTick::getTick() - tickstart) > _SPI_TIMEOUT_VALUE) {
				error_code |= _SPI_ERR_TIMEOUT; break;
			}
		}
		if (!waitFlag(SPI_SR_EOT_Pos, true, tickstart, _SPI_TIMEOUT_VALUE)) error_code |= _SPI_ERR_TIMEOUT;
		closeTransfer();
		state = SPIXState::Ready;
		return error_code ? 0 : size;
	}

	// AKA HAL_SPI_Receive
	stduint SPI_HARD::Receive(byte* rx, stduint size, IOMethod method) {
		if (!rx || !size) return 0;
		const stduint ds = _IMM(datasize);
		const bool master_2lines = (mode == SPIMode_E::Master) && (direction == SPIDirection_E::TwoLine);
		switch (method) {
		case IOMethod::Rupt: {
			// AKA HAL_SPI_Receive_IT
			if (state != SPIXState::Ready) return 0;
			state = SPIXState::BusyRX; error_code = 0;
			rx_buff = rx; rx_size = size; rx_count = size;
			tx_buff = nullptr; tx_size = 0; tx_count = 0;
			self[SPIReg::CR2].maset(SPI_CR2_TSIZE_Pos, 16, size);
			enAble(true);
			self[SPIReg::IER] = SPI_IER_EOTIE | SPI_IER_RXPIE | SPI_IER_UDRIE | SPI_IER_TIFREIE | SPI_IER_MODFIE;
			if (mode == SPIMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
			return size;
		}
		case IOMethod::DMA: {
			// AKA HAL_SPI_Receive_DMA
			if (state != SPIXState::Ready) return 0;
			state = SPIXState::BusyRX; error_code = 0;
			rx_buff = rx; rx_size = size; rx_count = size;
			tx_buff = nullptr; tx_size = 0; tx_count = 0;
			self[SPIReg::CR2].maset(SPI_CR2_TSIZE_Pos, 16, size);
			volatile uint32* rxdr = (volatile uint32*)(spin_addr[id - 1] + _IMM(SPIReg::RXDR));
			stduint ndtr = size / (ds > 15 ? 4 : (ds > 7 ? 2 : 1));
			self[SPIReg::CFG1].setof(SPI_CFG1_RXDMAEN_Pos);
			enAble(true);
			self[SPIReg::IER] = SPI_IER_EOTIE | SPI_IER_UDRIE | SPI_IER_TIFREIE | SPI_IER_MODFIE;
			if (mode == SPIMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
			if (!dmaRxStart((pureptr_t)rxdr, (pureptr_t)rx, ndtr)) {
				self[SPIReg::CFG1].rstof(SPI_CFG1_RXDMAEN_Pos);
				state = SPIXState::Ready; return 0;
			}
			return size;
		}
		case IOMethod::Loop: break;
		}
		// Loop (AKA HAL_SPI_Receive)
		if (state != SPIXState::Ready) return 0;
		state = SPIXState::BusyRX; error_code = 0;
		rx_buff = rx; rx_size = size; rx_count = size;
		tx_buff = nullptr; tx_size = 0; tx_count = 0;
		self[SPIReg::CR2].maset(SPI_CR2_TSIZE_Pos, 16, size);
		enAble(true);
		if (mode == SPIMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		uint64 tickstart = SysTick::getTick();
		while (rx_count > 0) {
			if (master_2lines && self[SPIReg::SR].bitof(SPI_SR_TXP_Pos)) {
				self[SPIReg::TXDR] = 0xFF; // dummy to generate clock
			}
			if (self[SPIReg::SR].bitof(SPI_SR_RXWNE_Pos)) {
				if (ds > 15) { *(uint32*)rx_buff = *(volatile uint32*)(spin_addr[id - 1] + _IMM(SPIReg::RXDR)); rx_buff += 4; rx_count--; }
				else if (ds > 7) { *(uint16*)rx_buff = *(volatile uint16*)(spin_addr[id - 1] + _IMM(SPIReg::RXDR)); rx_buff += 2; rx_count--; }
				else { *rx_buff = *(volatile uint8*)(spin_addr[id - 1] + _IMM(SPIReg::RXDR)); rx_buff += 1; rx_count--; }
			}
			else if ((SysTick::getTick() - tickstart) > _SPI_TIMEOUT_VALUE) {
				error_code |= _SPI_ERR_TIMEOUT; break;
			}
		}
		if (!waitFlag(SPI_SR_EOT_Pos, true, tickstart, _SPI_TIMEOUT_VALUE)) error_code |= _SPI_ERR_TIMEOUT;
		closeTransfer();
		state = SPIXState::Ready;
		return error_code ? 0 : size;
	}

	// AKA HAL_SPI_TransmitReceive
	stduint SPI_HARD::Transceive(const byte* tx, byte* rx, stduint size, IOMethod method) {
		if (!tx || !rx || !size) return 0;
		const stduint ds = _IMM(datasize);
		volatile uint32* txdr = (volatile uint32*)(spin_addr[id - 1] + _IMM(SPIReg::TXDR));
		volatile uint32* rxdr = (volatile uint32*)(spin_addr[id - 1] + _IMM(SPIReg::RXDR));
		switch (method) {
		case IOMethod::Rupt: {
			// AKA HAL_SPI_TransmitReceive_IT
			if (state != SPIXState::Ready) return 0;
			state = SPIXState::BusyTxRx; error_code = 0;
			tx_buff = tx; tx_size = size; tx_count = size;
			rx_buff = rx; rx_size = size; rx_count = size;
			self[SPIReg::CR2].maset(SPI_CR2_TSIZE_Pos, 16, size);
			enAble(true);
			self[SPIReg::IER] = SPI_IER_EOTIE | SPI_IER_TXPIE | SPI_IER_RXPIE | SPI_IER_UDRIE | SPI_IER_TIFREIE | SPI_IER_MODFIE;
			if (mode == SPIMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
			return size;
		}
		case IOMethod::DMA: {
			// AKA HAL_SPI_TransmitReceive_DMA
			if (state != SPIXState::Ready) return 0;
			state = SPIXState::BusyTxRx; error_code = 0;
			tx_buff = tx; tx_size = size; tx_count = size;
			rx_buff = rx; rx_size = size; rx_count = size;
			self[SPIReg::CR2].maset(SPI_CR2_TSIZE_Pos, 16, size);
			stduint ndtr = size / (ds > 15 ? 4 : (ds > 7 ? 2 : 1));
			self[SPIReg::CFG1].setof(SPI_CFG1_TXDMAEN_Pos);
			if (!dmaTxStart((pureptr_t)tx, (pureptr_t)txdr, ndtr)) {
				self[SPIReg::CFG1].rstof(SPI_CFG1_TXDMAEN_Pos);
				state = SPIXState::Ready; return 0;
			}
			self[SPIReg::CFG1].setof(SPI_CFG1_RXDMAEN_Pos);
			if (!dmaRxStart((pureptr_t)rxdr, (pureptr_t)rx, ndtr)) {
				self[SPIReg::CFG1].rstof(SPI_CFG1_RXDMAEN_Pos);
				state = SPIXState::Ready; return 0;
			}
			enAble(true);
			self[SPIReg::IER] = SPI_IER_EOTIE | SPI_IER_UDRIE | SPI_IER_TIFREIE | SPI_IER_MODFIE;
			if (mode == SPIMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
			return size;
		}
		case IOMethod::Loop: break;
		}
		// Loop (AKA HAL_SPI_TransmitReceive)
		if (state != SPIXState::Ready) return 0;
		state = SPIXState::BusyTxRx; error_code = 0;
		tx_buff = tx; tx_size = size; tx_count = size;
		rx_buff = rx; rx_size = size; rx_count = size;
		self[SPIReg::CR2].maset(SPI_CR2_TSIZE_Pos, 16, size);
		enAble(true);
		if (mode == SPIMode_E::Master) self[SPIReg::CR1].setof(SPI_CR1_CSTART_Pos);
		uint64 tickstart = SysTick::getTick();
		while (tx_count > 0 || rx_count > 0) {
			if (tx_count > 0 && self[SPIReg::SR].bitof(SPI_SR_TXP_Pos)) {
				if (ds > 15) { *txdr = *(const uint32*)tx_buff; tx_buff += 4; tx_count--; }
				else if (ds > 7) {
					if (tx_count > 1 && _IMM(fifo_threshold) > 0x00) { *txdr = *(const uint32*)tx_buff; tx_buff += 4; tx_count -= 2; }
					else { *(volatile uint16*)txdr = *(const uint16*)tx_buff; tx_buff += 2; tx_count--; }
				}
				else {
					if (tx_count > 3 && _IMM(fifo_threshold) > 0x40) { *txdr = *(const uint32*)tx_buff; tx_buff += 4; tx_count -= 4; }
					else if (tx_count > 1 && _IMM(fifo_threshold) > 0x00) { *(volatile uint16*)txdr = *(const uint16*)tx_buff; tx_buff += 2; tx_count -= 2; }
					else { *(volatile uint8*)txdr = *tx_buff; tx_buff += 1; tx_count--; }
				}
			}
			if (rx_count > 0 && self[SPIReg::SR].bitof(SPI_SR_RXWNE_Pos)) {
				if (ds > 15) { *(uint32*)rx_buff = *rxdr; rx_buff += 4; rx_count--; }
				else if (ds > 7) { *(uint16*)rx_buff = *(volatile uint16*)rxdr; rx_buff += 2; rx_count--; }
				else { *rx_buff = *(volatile uint8*)rxdr; rx_buff += 1; rx_count--; }
			}
			if ((SysTick::getTick() - tickstart) > _SPI_TIMEOUT_VALUE) {
				error_code |= _SPI_ERR_TIMEOUT; break;
			}
		}
		if (!waitFlag(SPI_SR_EOT_Pos, true, tickstart, _SPI_TIMEOUT_VALUE)) error_code |= _SPI_ERR_TIMEOUT;
		closeTransfer();
		state = SPIXState::Ready;
		return error_code ? 0 : size;
	}

	#if defined(_MPU_STM32MP13)
	// AKA HAL_SPI_Reload_Transmit_IT
	stduint SPI_HARD::ReloadTransmit(const byte* tx, stduint size) {
		if (reload_requested || !tx || !size) return 0;
		if (state != SPIXState::BusyTX) return 0;
		self[SPIReg::CR2].maset(16, 16, size);// CR2.TSER = reload count
		reload_requested = true;
		reload_tx = tx; reload_tx_size = size;
		return size;
	}
	// AKA HAL_SPI_Reload_Receive_IT
	stduint SPI_HARD::ReloadReceive(byte* rx, stduint size) {
		if (reload_requested || !rx || !size) return 0;
		if (state != SPIXState::BusyRX) return 0;
		self[SPIReg::CR2].maset(16, 16, size);
		reload_requested = true;
		reload_rx = rx; reload_rx_size = size;
		return size;
	}
	// AKA HAL_SPI_Reload_TransmitReceive_IT
	stduint SPI_HARD::ReloadTransceive(const byte* tx, byte* rx, stduint size) {
		if (reload_requested || !tx || !rx || !size) return 0;
		if (state != SPIXState::BusyTxRx) return 0;
		self[SPIReg::CR2].maset(16, 16, size);
		reload_requested = true;
		reload_tx = tx; reload_tx_size = size;
		reload_rx = rx; reload_rx_size = size;
		return size;
	}
	#endif

	// AKA HAL_SPI_Abort (blocking)
	bool SPI_HARD::Abort() {
		self[SPIReg::IER] = 0;
		self[SPIReg::CFG1] &= ~(SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);
		self[SPIReg::IFCR] = SPI_IFCR_EOTC | SPI_IFCR_TXTFC | SPI_IFCR_UDRC | SPI_IFCR_OVRC | SPI_IFCR_MODFC | SPI_IFCR_TIFREC;
		enAble(false);
		tx_count = 0; rx_count = 0;
		state = SPIXState::Ready;
		return true;
	}
	// AKA HAL_SPI_Abort_IT (non-blocking)
	bool SPI_HARD::AbortRupt() {
		self[SPIReg::IER] = 0;
		self[SPIReg::CFG1] &= ~(SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);
		self[SPIReg::IFCR] = SPI_IFCR_EOTC | SPI_IFCR_TXTFC | SPI_IFCR_UDRC | SPI_IFCR_OVRC | SPI_IFCR_MODFC | SPI_IFCR_TIFREC;
		enAble(false);
		tx_count = 0; rx_count = 0;
		state = SPIXState::Ready;
		if (AbortCpltHandler) AbortCpltHandler();
		return true;
	}

	// AKA HAL_SPI_DMAPause / DMAResume / DMAStop (not supported on H7/MP13, return error)
	bool SPI_HARD::PauseDMA() { setError(_SPI_ERR_NOT_SUPPORTED); return false; }
	bool SPI_HARD::ResumeDMA() { setError(_SPI_ERR_NOT_SUPPORTED); return false; }
	bool SPI_HARD::StopDMA() { setError(_SPI_ERR_NOT_SUPPORTED); return false; }

	// AKA HAL_SPIEx_FlushRxFifo
	bool SPI_HARD::FlushRxFifo() {
		enClock();
		// Disable then re-enable to flush RX FIFO
		enAble(false);
		enAble(true);
		self[SPIReg::IFCR] = SPI_IFCR_EOTC | SPI_IFCR_TXTFC | SPI_IFCR_UDRC | SPI_IFCR_OVRC | SPI_IFCR_MODFC | SPI_IFCR_TIFREC;
		return true;
	}
	// AKA HAL_SPIEx_ConfigureUnderrun
	bool SPI_HARD::ConfigureUnderrun(SPIUnderrunDetect_E det, SPIUnderrunBehav_E beh) {
		enClock();
		self[SPIReg::CFG1].maset(SPI_CFG1_UDRDET_Pos, 2, _IMM(det) >> SPI_CFG1_UDRDET_Pos);
		self[SPIReg::CFG1].maset(SPI_CFG1_UDRCFG_Pos, 2, _IMM(beh) >> SPI_CFG1_UDRCFG_Pos);
		return true;
	}
	// AKA HAL_SPIEx_EnableLockConfiguration
	bool SPI_HARD::EnableLockConfiguration() {
		enClock();
		self[SPIReg::CR1].setof(SPI_CR1_IOLOCK_Pos);
		return true;
	}

	// AKA SPI_RxISR: interrupt receive path
	void SPI_HARD::innHandlerByInterrupt() {
		const stduint ds = _IMM(datasize);
		volatile uint32* rxdr = (volatile uint32*)(spin_addr[id - 1] + _IMM(SPIReg::RXDR));
		while (self[SPIReg::SR].bitof(SPI_SR_RXWNE_Pos)) {
			if (rx_count == 0) {
			#if defined(_MPU_STM32MP13)
				if (reload_requested && reload_rx) {
					rx_buff = reload_rx; rx_size = reload_rx_size; rx_count = reload_rx_size;
					if (state == SPIXState::BusyRX) reload_requested = false;
				}
				else
			#endif
					break;
			}
			if (ds > 15) { *(uint32*)rx_buff = *rxdr; rx_buff += 4; rx_count--; }
			else if (ds > 7) { *(uint16*)rx_buff = *(volatile uint16*)rxdr; rx_buff += 2; rx_count--; }
			else { *rx_buff = *(volatile uint8*)rxdr; rx_buff += 1; rx_count--; }
		}
	}
	// AKA SPI_TxISR: interrupt transmit path
	void SPI_HARD::outHandlerByInterrupt() {
		const stduint ds = _IMM(datasize);
		volatile uint32* txdr = (volatile uint32*)(spin_addr[id - 1] + _IMM(SPIReg::TXDR));
		while (self[SPIReg::SR].bitof(SPI_SR_TXP_Pos)) {
			if (tx_count == 0) {
			#if defined(_MPU_STM32MP13)
				if (reload_requested && reload_tx) {
					tx_buff = reload_tx; tx_size = reload_tx_size; tx_count = reload_tx_size;
					if (state == SPIXState::BusyTX) reload_requested = false;
				}
				else
			#endif
					break;
			}
			if (ds > 15) { *txdr = *(const uint32*)tx_buff; tx_buff += 4; tx_count--; }
			else if (ds > 7) {
				if (tx_count > 1 && _IMM(fifo_threshold) > 0x00) { *txdr = *(const uint32*)tx_buff; tx_buff += 4; tx_count -= 2; }
				else { *(volatile uint16*)txdr = *(const uint16*)tx_buff; tx_buff += 2; tx_count--; }
			}
			else {
				if (tx_count > 3 && _IMM(fifo_threshold) > 0x40) { *txdr = *(const uint32*)tx_buff; tx_buff += 4; tx_count -= 4; }
				else if (tx_count > 1 && _IMM(fifo_threshold) > 0x00) { *(volatile uint16*)txdr = *(const uint16*)tx_buff; tx_buff += 2; tx_count -= 2; }
				else { *(volatile uint8*)txdr = *tx_buff; tx_buff += 1; tx_count--; }
			}
		}
	}

#if defined(_MCU_STM32H7x)
	SPI_HARD SPI1(1), SPI2(2), SPI3(3), SPI4(4), SPI5(5), SPI6(6);
#elif defined(_MPU_STM32MP13)
	SPI_HARD SPI1(1), SPI2(2), SPI3(3), SPI4(4), SPI5(5);
#endif

#endif


#endif
}


