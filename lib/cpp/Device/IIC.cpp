// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Protocol) Inter-Integrated Circuit, I2C
// Codifiers: @dosconio: 20240429~;
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
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
#define _MCU_RCC_TEMP
#define _MCU_IIC_TEMP
#include "../../../inc/cpp/Device/IIC"
#include "../../../inc/cpp/Device/RCC/RCC"
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
#include "../../../inc/cpp/Device/SysTick"
#include "../../../inc/cpp/Device/DMA"
#include "../../../inc/cpp/Device/BDMA"

namespace uni {
#if defined(_SUPPORT_GPIO)

	void IIC_SOFT::SendStart(stduint addr) {
		errcode = ERR_IIC_NONE;
		if (push_pull) SDA.setMode(GPIOMode::OUT_PushPull);
		SDA = true;
		SCL = true;
		asserv(func_delay)();
		SDA = false;
		asserv(func_delay)();
		SCL = false;
		if (addr) IIC_t::Send((byte)(addr << 1), true);// 7-bit addr << 1 | W, wait ACK
	}

	void IIC_SOFT::SendStop(void) {
		if (push_pull) SDA.setMode(GPIOMode::OUT_PushPull);
		SCL = false;// opt?
		SDA = false;
		asserv(func_delay)();
		SCL = true;
		SDA = true;
		asserv(func_delay)();
	}

	bool IIC_SOFT::WaitAcknowledge() {
		if (push_pull) SDA.setMode(GPIOMode::OUT_PushPull);
		byte timespan = 0;
		SDA = true;
		asserv(func_delay)();
		if (push_pull) SDA.setMode(GPIOMode::IN_Floating);
		SCL = true;
		asserv(func_delay)();
		while (SDA) {
			if (++timespan > _TEMP 250) {
				errcode |= ERR_IIC_NACK;
				SendStop();
				return last_ack_accepted = false;
			}
			asserv(func_delay)();
		}
		SCL = false;
		return last_ack_accepted = true;
	}
	void IIC_SOFT::SendAcknowledge(bool ack) {
		if (push_pull) SDA.setMode(GPIOMode::OUT_PushPull);
		SCL = false;
		SDA = !ack;
		asserv(func_delay)();
		SCL = true;
		asserv(func_delay)();
		SCL = false;
	}

	void IIC_SOFT::Send(byte* txtp, stduint len, bool auto_wait_ack) {
		if (push_pull) SDA.setMode(GPIOMode::OUT_PushPull);
		SCL = false;
		byte txt = nil;
		for0(i, len) {
			if (!(i & 0b111)) txt = *txtp++;
			SDA = txt & 0x80;
			txt <<= 1;
			asserv(func_delay)();// necessary delay
			SCL = true;
			asserv(func_delay)();
			SCL = false;
			asserv(func_delay)();
		}
		if (auto_wait_ack) WaitAcknowledge();
	}

	byte IIC_SOFT::ReadByte(bool feedback, bool ack) {
		if (push_pull) SDA.setMode(GPIOMode::IN_Floating);
		byte res = 0;
		for0(i, _BYTE_BITS_) {
			SCL = false;
			asserv(func_delay)();
			SCL = true;
			res <<= 1;
			if (SDA) res++;
			asserv(func_delay)();
		}
		SCL = false;// dosconio fix
		if (feedback) SendAcknowledge(ack);
		return res;
	}

	// ---- ---- ---- ---- HARD ---- ---- ---- ---- //

	#if defined(_MCU_STM32H7x)
	IIC_HARD IIC1(1), IIC2(2), IIC3(3), IIC4(4);
	#elif defined(_MPU_STM32MP13)
	IIC_HARD IIC1(1), IIC2(2), IIC3(3), IIC4(4), IIC5(5);
	#endif

	//

	#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
	#if defined(_MCU_STM32H7x)
	static const stduint iicn_addr[4] = {
		D2_APB1PERIPH_BASE + 0x5400,
		D2_APB1PERIPH_BASE + 0x5800,
		D2_APB1PERIPH_BASE + 0x5C00,
		D3_APB1PERIPH_BASE + 0x1C00
	};
	#elif defined(_MPU_STM32MP13)
	static const stduint iicn_addr[5] = {
		APB1_PERIPH_BASE + 0x12000,
		APB1_PERIPH_BASE + 0x13000,
		APB6_PERIPH_BASE + 0x4000,
		APB6_PERIPH_BASE + 0x5000,
		APB6_PERIPH_BASE + 0x6000
	};
	#endif

	
	Reference IIC_HARD::operator[](IICReg reg) {
		return iicn_addr[id - 1] + _IMMx4(reg);
	}

	#if defined(_MCU_STM32H7x)
	static GPIN* hSCLx[4]{ &GPIOB[6], &GPIOF[1], &GPIOH[7], &GPIOF[14] };
	static GPIN* hSDAx[4]{ &GPIOB[7], &GPIOF[0], &GPIOH[8], &GPIOF[15] };
	#elif defined(_MPU_STM32MP13)
	//{?} defaults = first option of each; alternatives kept in comment (datasheet AF table)
	static GPIN* hSCLx[5]{ &GPIOB[8], &GPIOD[7], &GPIOB[8], &GPIOB[13], &GPIOA[11] };// I2C1 PB8/PC10/PD12 · I2C2 PD7/PF2 · I2C3 PB8/PH3/PH12 · I2C4 PB13/PE2/PE15/PH11 · I2C5 PA11/PD1/PH13
	static GPIN* hSDAx[5]{ &GPIOC[11], &GPIOF[1], &GPIOD[7], &GPIOA[8], &GPIOE[13] };// I2C1 PC11/PD3/PE8 · I2C2 PF1/PG3/PG9 · I2C3 PD7/PD14/PH7/PH8/PH14 · I2C4 PA8/PB7/PB9 · I2C5 PE13/PF3/PH6
	static const byte afSCL[5]{ 4, 4, 5, 6, 4 };
	static const byte afSDA[5]{ 4, 4, 5, 4, 4 };
	#endif

	enum class XferMode_E : stduint { Reload = I2C_CR2_RELOAD, AutoEnd = I2C_CR2_AUTOEND, SoftEnd = 0 };
	enum class XferRequest_E : stduint {
		Stop,
		StartRead = 0x80000000U | I2C_CR2_START | I2C_CR2_RD_WRN,
		StartWrite = 0x80000000U | I2C_CR2_START,
		StartStop = 0x80000000U | I2C_CR2_STOP
	};
	
	bool IIC_HARD::enClock(bool ena) {
		#if defined(_MCU_STM32H7x)
		asrtret(Ranglin(id, 1, 4));
		auto reg = RCCReg::APB1LENR;
		if (id == 4) {
			reg = RCCReg::APB4ENR;
			RCC[reg].setof(7, ena);
			return RCC[reg].bitof(7);
		}
		else {
			RCC[reg].setof(21 - 1 + id, ena);
			return RCC[reg].bitof(21 - 1 + id);
		}
		#elif defined(_MPU_STM32MP13)
		asrtret(Ranglin(id, 1, 5));
		stduint refaddr;
		byte width;
		if (Ranglin(id, 1, 2)) {
			refaddr = _RCC_ADDR + (ena ? 0x700 : 0x704); // APB1ENSETR / APB1ENCLRR
			width = 21 - 1 + id; // I2C1=21, I2C2=22
		}
		else { // I2C3..5
			refaddr = _RCC_ADDR + (ena ? 0x748 : 0x74C); // APB6ENSETR / APB6ENCLRR
			width = 4 - 3 + id; // I2C3=4, I2C4=5, I2C5=6
		}
		Reference(refaddr).operator=(_IMM1S(width));
		return true;
		#endif
	}
	bool IIC_HARD::enAble(bool ena) {
		I2C_CR1_PE(self) = ena;
		return true;
	}
	bool IIC_HARD::setAnalogFilter(bool ena) {
		enAble(false);// I2C must be disabled to change ANFOFF
		if (ena) self[IICReg::CR1] &= ~_IMM(I2C_CR1_ANFOFF);// filter ON
		else     self[IICReg::CR1] |= I2C_CR1_ANFOFF;        // filter OFF
		enAble(true);
		return true;
	}
	bool IIC_HARD::setDigitalFilter(byte coeff) {
		if (coeff > 0x0F) return false;// AKA IS_I2C_DIGITAL_FILTER
		enAble(false);
		stduint tmpreg = self[IICReg::CR1];
		tmpreg &= ~I2C_CR1_DNF;
		tmpreg |= coeff << 8U;
		self[IICReg::CR1] = tmpreg;
		enAble(true);
		return true;
	}
	bool IIC_HARD::setWakeUp(bool ena) {
		enAble(false);
		if (ena) self[IICReg::CR1] |= I2C_CR1_WUPEN;
		else     self[IICReg::CR1] &= ~_IMM(I2C_CR1_WUPEN);
		enAble(true);
		return true;
	}
	bool IIC_HARD::setFastModePlus(bool ena) {
		#if defined(_MCU_STM32H7x)
		// AKA __HAL_RCC_SYSCFG_CLK_ENABLE (APB4ENR SYSCFGEN)
		RCC_APB4ENR_SYSCFGEN = true;
		// AKA SYSCFG->PMCR (SYSCFG_BASE = D3_APB1PERIPH_BASE + 0x0400, PMCR offset 0x04)
		Reference(D3_APB1PERIPH_BASE + 0x0404).setof(id - 1, ena);
		#elif defined(_MPU_STM32MP13)
		// SYSCFG clock: non-secure APB3ENSETR (0x720) bit 0 SYSCFGEN
		Reference(_RCC_ADDR + 0x720).operator=(_IMM1S(0));
		// AKA SYSCFG->PMCSETR (APB3_PERIPH_BASE + 0x04) / PMCCLRR (0x08), write-1-set/clear
		if (ena) Reference(APB3_PERIPH_BASE + 0x04).operator=(_IMM1S(id - 1));
		else     Reference(APB3_PERIPH_BASE + 0x08).operator=(_IMM1S(id - 1));
		#endif
		return true;
	}

	bool IIC_HARD::setMode(stduint OwnAddress1) {
		#if defined(_MCU_STM32H7x)
		asrtret(Ranglin(id, 1, 4)); byte _id = id - 1;
		#elif defined(_MPU_STM32MP13)
		asrtret(Ranglin(id, 1, 5)); byte _id = id - 1;
		#endif

		enum class AddressingMode_E { BITS7 = 0b01, BITS10 };
		
		_TEMP _Comment("Parameters > I2C_InitTypeDef")
		stduint Timing = 0x2000090E;// Frequency range
		AddressingMode_E AddressingMode = _TEMP AddressingMode_E::BITS7;
		bool DualAddressMode = false;
		int OwnAddress2 = 0;
		int OwnAddress2Masks = (_TEMP 0) & 0b111;// 0 .. 8, 0 is none
		bool GeneralCallMode = _TEMP false;
		bool NoStretchMode = _TEMP false;
		#if defined(_MCU_STM32H7x)
		IIC_Clksrc_E clksrc = _TEMP IIC_Clksrc_E::DxPCLK1;
		#endif
		
		{
			#if defined(_MCU_STM32H7x)
			// PeriphClkInitStruct.I2c123ClockSelection = RCC_I2C123CLKSOURCE_D2PCLK1; HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
			{
				if (clksrc == IIC_Clksrc_E::PLL3) {
					_TODO; //{TODO} RCCEx_PLL3_Config(&(PeriphClkInit->PLL3), DIVIDER_R_UPDATE);
					//      - Here we only do what related to R channel
				}
				else if (Ranglin(id, 1, 3)) {
					RCC_D2CCIP2R_I2C123SEL = _IMM(clksrc);
				}
				else {
					RCC_D3CCIPR_I2C4SEL = _IMM(clksrc);
				}
			}
			const byte&& alter_num_iic = 0x04;
			hSCLx[_id]->setMode(GPIOMode::OUT_AF_OpenDrain)._set_alternate(alter_num_iic);
			hSDAx[_id]->setMode(GPIOMode::OUT_AF_OpenDrain)._set_alternate(alter_num_iic);
			#elif defined(_MPU_STM32MP13)
			// kernel clock defaults to PCLK1 (I2C12CKSELR / I2C345CKSELR reset value 0)
			hSCLx[_id]->setMode(GPIOMode::OUT_AF_OpenDrain)._set_alternate(afSCL[_id]);
			hSDAx[_id]->setMode(GPIOMode::OUT_AF_OpenDrain)._set_alternate(afSDA[_id]);
			#endif
			enClock();
		}
		enAble(false);

		self[IICReg::TIMINGR] = Timing & TIMING_CLEAR_MASK;

		/*---------------------------- I2Cx OAR1 Configuration ---------------------*/
		/* Disable Own Address1 before set the Own Address1 configuration */
		self[IICReg::OAR1] &= ~I2C_OAR1_OA1EN;
		/* Configure I2Cx: Own Address1 and ack own address1 mode */
		// 7b or 10b
		self[IICReg::OAR1] = I2C_OAR1_OA1EN | OwnAddress1 |
			(AddressingMode == AddressingMode_E::BITS7 ? nil : I2C_OAR1_OA1MODE);

		/*---------------------------- I2Cx CR2 Configuration ----------------------*/
		/* Configure I2Cx: Addressing Master mode */
		if (AddressingMode == AddressingMode_E::BITS10)
		{
			self[IICReg::CR2] = (I2C_CR2_ADD10);
		}
		/* Enable the AUTOEND by default, and enable NACK (should be disable only during Slave process */
		self[IICReg::CR2] |= (I2C_CR2_AUTOEND | I2C_CR2_NACK);

		/*---------------------------- I2Cx OAR2 Configuration ---------------------*/
		/* Disable Own Address2 before set the Own Address2 configuration */
		self[IICReg::OAR2] &= ~I2C_OAR2_OA2EN;
		/* Configure I2Cx: Dual mode and Own Address2 */
		self[IICReg::OAR2] = (DualAddressMode ? I2C_OAR2_OA2EN : nil) |
			OwnAddress2 | (OwnAddress2Masks << 8);

		/*---------------------------- I2Cx CR1 Configuration ----------------------*/
		/* Configure I2Cx: Generalcall and NoStretch mode */
		self[IICReg::CR1] = (GeneralCallMode ? I2C_CR1_GCEN : nil) | (NoStretchMode ? I2C_CR1_NOSTRETCH : nil);

		enAble();
		return true;
	}

	void IIC_HARD::SendStart(stduint addr) {
		errcode = ERR_IIC_NONE;
		if (addr) dest_addr = addr;
		while (self[IICReg::ISR] & I2C_ISR_BUSY);
		send_start_just = true;
	}

	void IIC_HARD::SendStop(void) {
		/* No need to Check TC flag, with AUTOEND mode the stop is automatically generated */
		while (!(self[IICReg::ISR] & I2C_ISR_STOPF));
		self[IICReg::ICR] = I2C_ISR_STOPF; //__HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_STOPF);
		self[IICReg::CR2] &= ~_IMM(I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);//I2C_RESET_CR2(hi2c);
	}

	// !I2C_IsAcknowledgeFailed
	bool IIC_HARD::WaitAcknowledge() {
		static volatile stduint timeout = ~0;
		while (!(self[IICReg::ISR] & I2C_ISR_STOPF)) {
			--timeout;
			if (timeout == 0) return false;
		}
		self[IICReg::ICR] = I2C_ISR_NACKF; //__HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_AF);
		self[IICReg::ICR] = I2C_ISR_STOPF; //__HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_STOPF);
		// I2C_Flush_TXDR(hi2c);
		{
			/* If a pending TXIS flag is set */
			/* Write a dummy data in TXDR to clear it */
			if (self[IICReg::ISR] & I2C_ISR_TXIS) self[IICReg::TXDR] = 0x00U;
			/* Flush TX register if not empty */
			if (!(self[IICReg::ISR] & I2C_ISR_TXE)) self[IICReg::ISR] |= I2C_ISR_TXE;
		}
		self[IICReg::CR2] &= ~_IMM(I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);//I2C_RESET_CR2(hi2c);
		return true;
	}

	void IIC_HARD::SendAcknowledge(bool ack) {
		// AKA control CR2.NACK (slave ACK/NACK; master handled by hardware)
		if (ack) self[IICReg::CR2] &= ~_IMM(I2C_CR2_NACK);
		else     self[IICReg::CR2] |= I2C_CR2_NACK;
	}

	#define MAX_NBYTE_SIZE      255U
	void IIC_HARD::Send(byte* txt, stduint bitlen, bool auto_wait_ack) {
		if (role == IICRole::Slave) {
			// AKA HAL_I2C_Slave_Transmit (blocking)
			self[IICReg::CR2] &= ~_IMM(I2C_CR2_NACK);// enable address acknowledge
			if (!_WaitFlag(I2C_ISR_ADDR, true, I2C_TIMEOUT_ADDR, false)) { self[IICReg::CR2] |= I2C_CR2_NACK; return; }
			self[IICReg::ICR] = I2C_ISR_ADDR;
			if (!_WaitFlag(I2C_ISR_DIR, true, I2C_TIMEOUT_DIR, false)) { self[IICReg::CR2] |= I2C_CR2_NACK; return; }
			stduint n = (bitlen + _BYTE_BITS_ - 1) / _BYTE_BITS_;
			for0(i, n) {
				if (!_WaitFlag(I2C_ISR_TXIS, true, I2C_TIMEOUT_TXIS, true)) { self[IICReg::CR2] |= I2C_CR2_NACK; return; }
				self[IICReg::TXDR] = *txt++;
			}
			if (!_WaitFlag(I2C_ISR_STOPF, true, I2C_TIMEOUT_STOPF, true)) { self[IICReg::CR2] |= I2C_CR2_NACK; return; }
			self[IICReg::ICR] = I2C_ISR_STOPF;
			if (!_WaitFlag(I2C_ISR_BUSY, false, I2C_TIMEOUT_BUSY, false)) { self[IICReg::CR2] |= I2C_CR2_NACK; return; }
			self[IICReg::CR2] |= I2C_CR2_NACK;// disable address acknowledge
			return;
		}
		// self[IICReg::TXDR] = txt[0];
		// for (volatile int i = 200; i; i--);
		// return;
		//
		if (bitlen > MAX_NBYTE_SIZE * _BYTE_BITS_) return;
		if (send_start_just) {
			send_start_just = false;
			/* Send Slave Address and Memory Address */
			//if (I2C_RequestMemoryWrite(hi2c, DevAddress, MemAddress, MemAddSize, Timeout, tickstart) != HAL_OK)
			{
				stduint xfer_size = (bitlen + _BYTE_BITS_ - 1) / _BYTE_BITS_; MIN(xfer_size, MAX_NBYTE_SIZE);
				// I2C_TransferConfig(hi2c, DevAddress, MemAddSize, I2C_RELOAD_MODE, I2C_GENERATE_START_WRITE);
				{
					stduint cr2 = self[IICReg::CR2];
					cr2 &= ~_IMM((I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND | (I2C_CR2_RD_WRN & (_IMM(XferRequest_E::StartStop) >> (31U - I2C_CR2_RD_WRN_Pos))) | I2C_CR2_START | I2C_CR2_STOP));
					cr2 |= (dest_addr & I2C_CR2_SADD) | ((xfer_size << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) | _IMM(XferMode_E::Reload) | _IMM(XferRequest_E::StartWrite);
					self[IICReg::CR2] = cr2;
				}
				for (volatile int i = 200; i; i--);
				//while (!(self[IICReg::ISR] & I2C_ISR_TXIS))
				{ // I2C_WaitOnTXISFlagUntilTimeout
					//{} if (auto_wait_ack) if (WaitAcknowledge());
				}
				/* If Memory address size is 8Bit */
				if (bitlen == 8)
				{
					self[IICReg::TXDR] = txt[0];
				}
				/* If Memory address size is 16Bit */
				else
				{
					self[IICReg::TXDR] = txt[1];//[LE] aasume little endian
					for (volatile int i = 200; i; i--);
					//while (!(self[IICReg::ISR] & I2C_ISR_TXIS))
					{ // I2C_WaitOnTXISFlagUntilTimeout
						//{} if (auto_wait_ack) if (WaitAcknowledge());
					}
					self[IICReg::TXDR] = txt[0];
				}
				while (!(self[IICReg::ISR] & I2C_ISR_TCR));
			}

			/* Set NBYTES to write and reload if hi2c->XferCount > MAX_NBYTE_SIZE */
			stduint xfer_size = (bitlen + _BYTE_BITS_ - 1) / _BYTE_BITS_; MIN(xfer_size, MAX_NBYTE_SIZE);
			XferMode_E XferMode = bitlen > MAX_NBYTE_SIZE * _BYTE_BITS_ ? XferMode_E::Reload : XferMode_E::AutoEnd;
			//I2C_TransferConfig(hi2c, DevAddress, xfer_size, XferMode, XferRequest_E::StartStop);
			{
				stduint cr2 = self[IICReg::CR2];
				cr2 &= ~_IMM((I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND | (I2C_CR2_RD_WRN & (_IMM(XferRequest_E::StartStop) >> (31U - I2C_CR2_RD_WRN_Pos))) | I2C_CR2_START | I2C_CR2_STOP));
				cr2 |= (dest_addr & I2C_CR2_SADD) | ((xfer_size << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) | _IMM(XferMode) | _IMM(XferRequest_E::StartStop);
				self[IICReg::CR2] = cr2;
			}
		}
		else for0(i, (bitlen + 7) / 8) {
			for (volatile int i = 200; i; i--);
			//while (!(self[IICReg::ISR] & I2C_ISR_TXIS)) 
			{ // I2C_WaitOnTXISFlagUntilTimeout
				//{} if (auto_wait_ack) if (WaitAcknowledge());
			}
			{} // then if (hi2c->ErrorCode == HAL_I2C_ERROR_AF) ...;
			self[IICReg::TXDR] = *txt++;
			// no consider the case bitlen > MAX_NBYTE_SIZE * _BYTE_BITS_
		}
	}
	byte IIC_HARD::ReadByte(bool feedback, bool ack) {
		if (role == IICRole::Slave) {
			// AKA HAL_I2C_Slave_Receive (blocking, single byte)
			self[IICReg::CR2] &= ~_IMM(I2C_CR2_NACK);
			if (!_WaitFlag(I2C_ISR_ADDR, true, I2C_TIMEOUT_ADDR, false)) { self[IICReg::CR2] |= I2C_CR2_NACK; return 0; }
			self[IICReg::ICR] = I2C_ISR_ADDR;
			if (!_WaitFlag(I2C_ISR_DIR, false, I2C_TIMEOUT_DIR, false)) { self[IICReg::CR2] |= I2C_CR2_NACK; return 0; }
			if (!_WaitFlag(I2C_ISR_RXNE, true, I2C_TIMEOUT_RXNE, true)) { self[IICReg::CR2] |= I2C_CR2_NACK; return 0; }
			byte sres = self[IICReg::RXDR];
			if (!_WaitFlag(I2C_ISR_STOPF, true, I2C_TIMEOUT_STOPF, true)) { self[IICReg::CR2] |= I2C_CR2_NACK; return 0; }
			self[IICReg::ICR] = I2C_ISR_STOPF;
			self[IICReg::CR2] |= I2C_CR2_NACK;
			return sres;
		}
		// single-byte master read (AUTOEND = STOP); feedback/ack control CR2.NACK
		byte res = 0;
		errcode = ERR_IIC_NONE;
		if (!_WaitFlag(I2C_ISR_BUSY, false, I2C_TIMEOUT_BUSY, false)) { errcode |= ERR_IIC_TIMEOUT; return 0; }// AKA wait for BUSY clear (bus idle)
		if (!feedback || !ack) self[IICReg::CR2] |= I2C_CR2_NACK;
		else                   self[IICReg::CR2] &= ~_IMM(I2C_CR2_NACK);
		_TransferConfig(1, _IMM(XferMode_E::AutoEnd), _IMM(XferRequest_E::StartRead));
		if (!_WaitFlag(I2C_ISR_RXNE, true, I2C_TIMEOUT_RXNE, true)) return 0;
		res = self[IICReg::RXDR];
		if (!_WaitFlag(I2C_ISR_STOPF, true, I2C_TIMEOUT_STOPF, true)) return 0;
		_FinishXfer();
		return res;
	}

	void IIC_HARD::_TransferConfig(stduint size, stduint mode, stduint request) {
		stduint cr2 = self[IICReg::CR2];
		cr2 &= ~_IMM(I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND | (I2C_CR2_RD_WRN & (request >> (31U - I2C_CR2_RD_WRN_Pos))) | I2C_CR2_START | I2C_CR2_STOP);
		cr2 |= (dest_addr & I2C_CR2_SADD) | ((size << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) | mode | (request & ~0x80000000U);
		self[IICReg::CR2] = cr2;
	}

	// H7/MP13 DMAMUX1 request line IDs for I2C1~5 (H7 I2C4 via BDMA/DMAMUX2: RX=13/TX=14)
	static stduint iic_dmaRequestID(byte iic_id, bool is_tx) {
		#if defined(_MCU_STM32H7x)
		if (iic_id == 4) return is_tx ? 14U : 13U;// DMAMUX2 (BDMA)
		#endif
		static const stduint req_tbl[2][5] = {
			{ 33, 35, 73, 75, 115 }, // RX
			{ 34, 36, 74, 76, 116 }  // TX
		};
		if (!Ranglin(iic_id, 1, 5)) return 0;
		return req_tbl[is_tx ? 1 : 0][iic_id - 1];
	}
	static void iic_dmaTransmitCplt(void) {
		IIC_HARD* ic = nullptr;
		if (DMA1.XferCpltCallback == iic_dmaTransmitCplt) ic = (IIC_HARD*)DMA1.bind;
		else if (DMA2.XferCpltCallback == iic_dmaTransmitCplt) ic = (IIC_HARD*)DMA2.bind;
		#if defined(_MCU_STM32H7x)
		else if (BDMA.XferCpltCallback == iic_dmaTransmitCplt) ic = (IIC_HARD*)BDMA.bind;
		#endif
		if (!ic) return;
		ic->dmaTransmitCplt();
	}
	static void iic_dmaReceiveCplt(void) {
		IIC_HARD* ic = nullptr;
		if (DMA1.XferCpltCallback == iic_dmaReceiveCplt) ic = (IIC_HARD*)DMA1.bind;
		else if (DMA2.XferCpltCallback == iic_dmaReceiveCplt) ic = (IIC_HARD*)DMA2.bind;
		#if defined(_MCU_STM32H7x)
		else if (BDMA.XferCpltCallback == iic_dmaReceiveCplt) ic = (IIC_HARD*)BDMA.bind;
		#endif
		if (!ic) return;
		ic->dmaReceiveCplt();
	}
	// DMA start (H7 I2C4 routed to BDMA)
	bool IIC_HARD::_DmaTxStart(pureptr_t mem, pureptr_t peri, stduint ndtr) {
		#if defined(_MCU_STM32H7x)
		if (id == 4) {
			if (bdma_tx_channel > 7) return false;
			BDMA.bind = (pureptr_t)this;
			BDMA.XferCpltCallback = iic_dmaTransmitCplt;
			BDMA.setRequest(bdma_tx_channel, 14);// BDMA_REQUEST_I2C4_TX (DMAMUX2)
			return BDMA.Transfer(bdma_tx_channel, mem, peri, ndtr, IOMethod::Rupt);
		}
		#endif
		if (!hdmatx) return false;
		DMA_t& dma = hdmatx->getParent();
		dma.bind = (pureptr_t)this;
		dma.XferCpltCallback = iic_dmaTransmitCplt;
		return hdmatx->Transfer(mem, peri, ndtr, IOMethod::Rupt);
	}
	bool IIC_HARD::_DmaRxStart(pureptr_t peri, pureptr_t mem, stduint ndtr) {
		#if defined(_MCU_STM32H7x)
		if (id == 4) {
			if (bdma_rx_channel > 7) return false;
			BDMA.bind = (pureptr_t)this;
			BDMA.XferCpltCallback = iic_dmaReceiveCplt;
			BDMA.setRequest(bdma_rx_channel, 13);// BDMA_REQUEST_I2C4_RX (DMAMUX2)
			return BDMA.Transfer(bdma_rx_channel, peri, mem, ndtr, IOMethod::Rupt);
		}
		#endif
		if (!hdmarx) return false;
		DMA_t& dma = hdmarx->getParent();
		dma.bind = (pureptr_t)this;
		dma.XferCpltCallback = iic_dmaReceiveCplt;
		return hdmarx->Transfer(peri, mem, ndtr, IOMethod::Rupt);
	}

	stduint IIC_HARD::Transmit(const byte* tx, stduint size, IOMethod method, XferOption opt) {
		if (!tx || !size) { errcode |= ERR_IIC_SIZE; return 0; }
		if (method == IOMethod::DMA) {
			// AKA HAL_I2C_Master_Transmit_DMA (preload first byte + RELOAD chunking)
			if (!hdmatx || lock) return 0;
			lock = true;
			tx_ptr = tx;
			xfer_count = size;
			xfer_size = (size > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : size;
			dma_xfer = true;
			errcode = ERR_IIC_NONE;
			send_start_just = false;
			if (self[IICReg::ISR] & I2C_ISR_BUSY) { lock = false; dma_xfer = false; return 0; }// AKA HAL: BUSY==SET returns HAL_BUSY
			// preload first byte
			self[IICReg::TXDR] = *tx_ptr++;
			xfer_count--;
			xfer_size--;
			if (xfer_size > 0 && !_DmaTxStart((pureptr_t)tx_ptr, (pureptr_t)(iicn_addr[id - 1] + _IMMx4(IICReg::TXDR)), xfer_size)) {
				self[IICReg::CR1] &= ~_IMM(I2C_CR1_TXDMAEN);
				lock = false; dma_xfer = false; return 0;
			}
			// NBYTES = preload(1) + DMA(xfer_size)
			_TransferConfig(xfer_size + 1, size > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : _IMM(XferMode_E::AutoEnd), _IMM(XferRequest_E::StartWrite));
			xfer_count -= xfer_size;
			self[IICReg::CR1] |= I2C_CR1_TXDMAEN | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_ERRIE;
			return size;
		}
		errcode = ERR_IIC_NONE;
		send_start_just = false;
		if (method == IOMethod::Rupt) {
			// AKA HAL_I2C_Master_Transmit_IT / HAL_I2C_Master_Sequential_Transmit_IT
			lock = true;
			tx_ptr = tx;
			xfer_count = size;
			xfer_size = (size > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : size;
			dma_xfer = false;
			if (self[IICReg::ISR] & I2C_ISR_BUSY) { lock = false; return 0; }// AKA HAL: BUSY==SET returns HAL_BUSY
			const bool stop = (opt == XferOption::FirstAndLastFrame || opt == XferOption::LastFrame);
			xfer_stop = stop;
			const stduint request = (opt == XferOption::FirstAndLastFrame || opt == XferOption::FirstFrame) ? _IMM(XferRequest_E::StartWrite) : 0;
			_TransferConfig(xfer_size, size > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : (stop ? _IMM(XferMode_E::AutoEnd) : _IMM(XferMode_E::SoftEnd)), request);
			self[IICReg::CR1] |= I2C_CR1_TXIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_TCIE | I2C_CR1_ERRIE;
			return size;
		}
		// AKA I2C_WaitOnFlagUntilTimeout(I2C_FLAG_BUSY, SET, I2C_TIMEOUT_BUSY)
		{
			uint64 fstart = SysTick::getTick();
			while (self[IICReg::ISR] & I2C_ISR_BUSY) {
				if ((SysTick::getTick() - fstart) > I2C_TIMEOUT_BUSY) { errcode |= ERR_IIC_TIMEOUT; return 0; }
			}
		}
		stduint xfer_count = size;
		stduint xfer_size = (xfer_count > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : xfer_count;
		const bool stop = (opt == XferOption::FirstAndLastFrame || opt == XferOption::LastFrame);
		const stduint request = (opt == XferOption::FirstAndLastFrame || opt == XferOption::FirstFrame) ? _IMM(XferRequest_E::StartWrite) : 0;
		_TransferConfig(xfer_size, xfer_count > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : (stop ? _IMM(XferMode_E::AutoEnd) : _IMM(XferMode_E::SoftEnd)), request);
		const byte* p = tx;
		while (xfer_count > 0) {
			// AKA I2C_WaitOnTXISFlagUntilTimeout
			{
				uint64 fstart = SysTick::getTick();
				while (!(self[IICReg::ISR] & I2C_ISR_TXIS)) {
					if (self[IICReg::ISR] & I2C_ISR_NACKF) { errcode |= ERR_IIC_NACK; return 0; }// AKA AF
					if ((SysTick::getTick() - fstart) > I2C_TIMEOUT_TXIS) { errcode |= ERR_IIC_TIMEOUT; return 0; }
				}
			}
			self[IICReg::TXDR] = *p++;
			xfer_size--;
			xfer_count--;
			if (xfer_size == 0 && xfer_count != 0) {
				// AKA I2C_WaitOnFlagUntilTimeout(I2C_FLAG_TCR, RESET)
				{
					uint64 fstart = SysTick::getTick();
					while (!(self[IICReg::ISR] & I2C_ISR_TCR)) {
						if ((SysTick::getTick() - fstart) > I2C_TIMEOUT_TCR) { errcode |= ERR_IIC_TIMEOUT; return 0; }
					}
				}
				xfer_size = (xfer_count > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : xfer_count;
				_TransferConfig(xfer_size, xfer_count > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : (stop ? _IMM(XferMode_E::AutoEnd) : _IMM(XferMode_E::SoftEnd)), 0);// AKA I2C_NO_STARTSTOP
			}
		}
		if (stop) {
			// AKA I2C_WaitOnSTOPFlagUntilTimeout
			{
				uint64 fstart = SysTick::getTick();
				while (!(self[IICReg::ISR] & I2C_ISR_STOPF)) {
					if (self[IICReg::ISR] & I2C_ISR_NACKF) { errcode |= ERR_IIC_NACK; return 0; }
					if ((SysTick::getTick() - fstart) > I2C_TIMEOUT_STOPF) { errcode |= ERR_IIC_TIMEOUT; return 0; }
				}
			}
			self[IICReg::ICR] = I2C_ISR_STOPF;// AKA __HAL_I2C_CLEAR_FLAG(I2C_FLAG_STOPF)
			self[IICReg::CR2] &= ~_IMM(I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);// AKA I2C_RESET_CR2
		}
		else {
			// AKA I2C_WaitOnFlagUntilTimeout(I2C_FLAG_TC, RESET) — no STOP, bus held
			{
				uint64 fstart = SysTick::getTick();
				while (!(self[IICReg::ISR] & I2C_ISR_TC)) {
					if ((SysTick::getTick() - fstart) > I2C_TIMEOUT_TC) { errcode |= ERR_IIC_TIMEOUT; return 0; }
				}
			}
		}
		return size;
	}
	stduint IIC_HARD::Receive(byte* rx, stduint size, IOMethod method, XferOption opt) {
		if (!rx || !size) { errcode |= ERR_IIC_SIZE; return 0; }
		if (method == IOMethod::DMA) {
			// AKA HAL_I2C_Master_Receive_DMA (RELOAD chunking, no preload)
			if (!hdmarx || lock) return 0;
			lock = true;
			rx_ptr = rx;
			xfer_count = size;
			xfer_size = (size > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : size;
			dma_xfer = true;
			errcode = ERR_IIC_NONE;
			send_start_just = false;
			if (self[IICReg::ISR] & I2C_ISR_BUSY) { lock = false; dma_xfer = false; return 0; }// AKA HAL: BUSY==SET returns HAL_BUSY
			_TransferConfig(xfer_size, size > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : _IMM(XferMode_E::AutoEnd), _IMM(XferRequest_E::StartRead));
			if (!_DmaRxStart((pureptr_t)(iicn_addr[id - 1] + _IMMx4(IICReg::RXDR)), (pureptr_t)rx_ptr, xfer_size)) {
				self[IICReg::CR1] &= ~_IMM(I2C_CR1_RXDMAEN);
				lock = false; dma_xfer = false; return 0;
			}
			xfer_count -= xfer_size;
			self[IICReg::CR1] |= I2C_CR1_RXDMAEN | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_ERRIE;
			return size;
		}
		errcode = ERR_IIC_NONE;
		send_start_just = false;
		if (method == IOMethod::Rupt) {
			// AKA HAL_I2C_Master_Receive_IT / HAL_I2C_Master_Sequential_Receive_IT
			lock = true;
			rx_ptr = rx;
			xfer_count = size;
			xfer_size = (size > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : size;
			dma_xfer = false;
			if (self[IICReg::ISR] & I2C_ISR_BUSY) { lock = false; return 0; }// AKA HAL: BUSY==SET returns HAL_BUSY
			const bool stop = (opt == XferOption::FirstAndLastFrame || opt == XferOption::LastFrame);
			xfer_stop = stop;
			const stduint request = (opt == XferOption::FirstAndLastFrame || opt == XferOption::FirstFrame) ? _IMM(XferRequest_E::StartRead) : 0;
			_TransferConfig(xfer_size, size > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : (stop ? _IMM(XferMode_E::AutoEnd) : _IMM(XferMode_E::SoftEnd)), request);
			self[IICReg::CR1] |= I2C_CR1_RXIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_TCIE | I2C_CR1_ERRIE;
			return size;
		}
		// AKA I2C_WaitOnFlagUntilTimeout(I2C_FLAG_BUSY, SET, I2C_TIMEOUT_BUSY)
		{
			uint64 fstart = SysTick::getTick();
			while (self[IICReg::ISR] & I2C_ISR_BUSY) {
				if ((SysTick::getTick() - fstart) > I2C_TIMEOUT_BUSY) { errcode |= ERR_IIC_TIMEOUT; return 0; }
			}
		}
		stduint xfer_count = size;
		stduint xfer_size = (xfer_count > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : xfer_count;
		const bool stop = (opt == XferOption::FirstAndLastFrame || opt == XferOption::LastFrame);
		const stduint request = (opt == XferOption::FirstAndLastFrame || opt == XferOption::FirstFrame) ? _IMM(XferRequest_E::StartRead) : 0;
		_TransferConfig(xfer_size, xfer_count > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : (stop ? _IMM(XferMode_E::AutoEnd) : _IMM(XferMode_E::SoftEnd)), request);
		byte* p = rx;
		while (xfer_count > 0) {
			// AKA I2C_WaitOnRXNEFlagUntilTimeout
			{
				uint64 fstart = SysTick::getTick();
				while (!(self[IICReg::ISR] & I2C_ISR_RXNE)) {
					if (self[IICReg::ISR] & I2C_ISR_NACKF) { errcode |= ERR_IIC_NACK; return 0; }
					if ((SysTick::getTick() - fstart) > I2C_TIMEOUT_RXNE) { errcode |= ERR_IIC_TIMEOUT; return 0; }
				}
			}
			*p++ = self[IICReg::RXDR];
			xfer_size--;
			xfer_count--;
			if (xfer_size == 0 && xfer_count != 0) {
				// AKA I2C_WaitOnFlagUntilTimeout(I2C_FLAG_TCR, RESET)
				{
					uint64 fstart = SysTick::getTick();
					while (!(self[IICReg::ISR] & I2C_ISR_TCR)) {
						if ((SysTick::getTick() - fstart) > I2C_TIMEOUT_TCR) { errcode |= ERR_IIC_TIMEOUT; return 0; }
					}
				}
				xfer_size = (xfer_count > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : xfer_count;
				_TransferConfig(xfer_size, xfer_count > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : (stop ? _IMM(XferMode_E::AutoEnd) : _IMM(XferMode_E::SoftEnd)), 0);// AKA I2C_NO_STARTSTOP, keep RD_WRN
			}
		}
		if (stop) {
			// AKA I2C_WaitOnSTOPFlagUntilTimeout
			{
				uint64 fstart = SysTick::getTick();
				while (!(self[IICReg::ISR] & I2C_ISR_STOPF)) {
					if (self[IICReg::ISR] & I2C_ISR_NACKF) { errcode |= ERR_IIC_NACK; return 0; }
					if ((SysTick::getTick() - fstart) > I2C_TIMEOUT_STOPF) { errcode |= ERR_IIC_TIMEOUT; return 0; }
				}
			}
			self[IICReg::ICR] = I2C_ISR_STOPF;
			self[IICReg::CR2] &= ~_IMM(I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);
		}
		else {
			// AKA I2C_WaitOnFlagUntilTimeout(I2C_FLAG_TC, RESET) — no STOP, bus held
			{
				uint64 fstart = SysTick::getTick();
				while (!(self[IICReg::ISR] & I2C_ISR_TC)) {
					if ((SysTick::getTick() - fstart) > I2C_TIMEOUT_TC) { errcode |= ERR_IIC_TIMEOUT; return 0; }
				}
			}
		}
		return size;
	}

	bool IIC_HARD::_WaitFlag(stduint flag, bool expect_set, stduint timeout, bool check_nack) {
		uint64 fstart = SysTick::getTick();
		while (true) {
			bool is_set = (self[IICReg::ISR] & flag) != 0;
			if (is_set == expect_set) return true;
			if (check_nack && (self[IICReg::ISR] & I2C_ISR_NACKF)) { errcode |= ERR_IIC_NACK; return false; }// AKA AF
			if ((SysTick::getTick() - fstart) > timeout) { errcode |= ERR_IIC_TIMEOUT; return false; }
		}
	}
	void IIC_HARD::_FinishXfer(void) {
		self[IICReg::ICR] = I2C_ISR_STOPF;// AKA __HAL_I2C_CLEAR_FLAG(I2C_FLAG_STOPF)
		self[IICReg::CR2] &= ~_IMM(I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);// AKA I2C_RESET_CR2
	}
	stduint IIC_HARD::MemWrite(stduint mem_addr, MemAddrSize mem_size, const byte* tx, stduint size, IOMethod method) {
		if (!tx || !size) { errcode |= ERR_IIC_SIZE; return 0; }
		errcode = ERR_IIC_NONE;
		send_start_just = false;
		if (!_WaitFlag(I2C_ISR_BUSY, false, I2C_TIMEOUT_BUSY, false)) return 0;// AKA wait for BUSY clear (bus idle)
		// AKA I2C_RequestMemoryWrite: send mem addr in RELOAD mode
		_TransferConfig(_IMM(mem_size), _IMM(XferMode_E::Reload), _IMM(XferRequest_E::StartWrite));
		if (!_WaitFlag(I2C_ISR_TXIS, true, I2C_TIMEOUT_TXIS, true)) return 0;
		if (mem_size == MemAddrSize::Byte1) {
			self[IICReg::TXDR] = (byte)(mem_addr & 0xFFU);// LSB
		}
		else {
			self[IICReg::TXDR] = (byte)((mem_addr >> 8) & 0xFFU);// MSB
			if (!_WaitFlag(I2C_ISR_TXIS, true, I2C_TIMEOUT_TXIS, true)) return 0;
			self[IICReg::TXDR] = (byte)(mem_addr & 0xFFU);// LSB
		}
		if (!_WaitFlag(I2C_ISR_TCR, true, I2C_TIMEOUT_TCR, false)) return 0;
		// data phase
		if (method == IOMethod::Rupt) {
			lock = true;
			tx_ptr = tx;
			xfer_count = size;
			xfer_size = (size > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : size;
			_TransferConfig(xfer_size, size > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : _IMM(XferMode_E::AutoEnd), 0);
			self[IICReg::CR1] |= I2C_CR1_TXIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_TCIE | I2C_CR1_ERRIE;
			return size;
		}
		if (method == IOMethod::DMA) {
			// AKA HAL_I2C_Mem_Write_DMA (preload first byte + RELOAD chunking)
			if (!hdmatx) return 0;
			lock = true;
			tx_ptr = tx;
			xfer_count = size;
			xfer_size = (size > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : size;
			dma_xfer = true;
			// preload first byte
			self[IICReg::TXDR] = *tx_ptr++;
			xfer_count--;
			xfer_size--;
			if (xfer_size > 0 && !_DmaTxStart((pureptr_t)tx_ptr, (pureptr_t)(iicn_addr[id - 1] + _IMMx4(IICReg::TXDR)), xfer_size)) {
				self[IICReg::CR1] &= ~_IMM(I2C_CR1_TXDMAEN);
				lock = false; dma_xfer = false; return 0;
			}
			// NBYTES = preload(1) + DMA(xfer_size), NO_STARTSTOP
			_TransferConfig(xfer_size + 1, size > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : _IMM(XferMode_E::AutoEnd), 0);
			xfer_count -= xfer_size;
			self[IICReg::CR1] |= I2C_CR1_TXDMAEN | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_ERRIE;
			return size;
		}
		// data phase (NO_STARTSTOP, Loop)
		stduint xfer_count = size;
		stduint xfer_size = (xfer_count > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : xfer_count;
		_TransferConfig(xfer_size, xfer_count > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : _IMM(XferMode_E::AutoEnd), 0);
		const byte* p = tx;
		while (xfer_count > 0) {
			if (!_WaitFlag(I2C_ISR_TXIS, true, I2C_TIMEOUT_TXIS, true)) return 0;
			self[IICReg::TXDR] = *p++;
			xfer_size--;
			xfer_count--;
			if (xfer_size == 0 && xfer_count != 0) {
				if (!_WaitFlag(I2C_ISR_TCR, true, I2C_TIMEOUT_TCR, false)) return 0;
				xfer_size = (xfer_count > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : xfer_count;
				_TransferConfig(xfer_size, xfer_count > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : _IMM(XferMode_E::AutoEnd), 0);
			}
		}
		if (!_WaitFlag(I2C_ISR_STOPF, true, I2C_TIMEOUT_STOPF, true)) return 0;
		_FinishXfer();
		return size;
	}
	stduint IIC_HARD::MemRead(stduint mem_addr, MemAddrSize mem_size, byte* rx, stduint size, IOMethod method) {
		if (!rx || !size) { errcode |= ERR_IIC_SIZE; return 0; }
		errcode = ERR_IIC_NONE;
		send_start_just = false;
		if (!_WaitFlag(I2C_ISR_BUSY, false, I2C_TIMEOUT_BUSY, false)) return 0;// AKA wait for BUSY clear (bus idle)
		// AKA I2C_RequestMemoryRead: send mem addr in SOFTEND mode
		_TransferConfig(_IMM(mem_size), _IMM(XferMode_E::SoftEnd), _IMM(XferRequest_E::StartWrite));
		if (!_WaitFlag(I2C_ISR_TXIS, true, I2C_TIMEOUT_TXIS, true)) return 0;
		if (mem_size == MemAddrSize::Byte1) {
			self[IICReg::TXDR] = (byte)(mem_addr & 0xFFU);// LSB
		}
		else {
			self[IICReg::TXDR] = (byte)((mem_addr >> 8) & 0xFFU);// MSB
			if (!_WaitFlag(I2C_ISR_TXIS, true, I2C_TIMEOUT_TXIS, true)) return 0;
			self[IICReg::TXDR] = (byte)(mem_addr & 0xFFU);// LSB
		}
		if (!_WaitFlag(I2C_ISR_TC, true, I2C_TIMEOUT_TC, false)) return 0;
		// data phase
		if (method == IOMethod::Rupt) {
			lock = true;
			rx_ptr = rx;
			xfer_count = size;
			xfer_size = (size > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : size;
			_TransferConfig(xfer_size, size > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : _IMM(XferMode_E::AutoEnd), _IMM(XferRequest_E::StartRead));
			self[IICReg::CR1] |= I2C_CR1_RXIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_TCIE | I2C_CR1_ERRIE;
			return size;
		}
		if (method == IOMethod::DMA) {
			// AKA HAL_I2C_Mem_Read_DMA (RELOAD chunking, no preload)
			if (!hdmarx) return 0;
			lock = true;
			rx_ptr = rx;
			xfer_count = size;
			xfer_size = (size > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : size;
			dma_xfer = true;
			_TransferConfig(xfer_size, size > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : _IMM(XferMode_E::AutoEnd), _IMM(XferRequest_E::StartRead));
			if (!_DmaRxStart((pureptr_t)(iicn_addr[id - 1] + _IMMx4(IICReg::RXDR)), (pureptr_t)rx_ptr, xfer_size)) {
				self[IICReg::CR1] &= ~_IMM(I2C_CR1_RXDMAEN);
				lock = false; dma_xfer = false; return 0;
			}
			xfer_count -= xfer_size;
			self[IICReg::CR1] |= I2C_CR1_RXDMAEN | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_ERRIE;
			return size;
		}
		// data phase (RESTART read, Loop)
		stduint xfer_count = size;
		stduint xfer_size = (xfer_count > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : xfer_count;
		_TransferConfig(xfer_size, xfer_count > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : _IMM(XferMode_E::AutoEnd), _IMM(XferRequest_E::StartRead));
		byte* p = rx;
		while (xfer_count > 0) {
			if (!_WaitFlag(I2C_ISR_RXNE, true, I2C_TIMEOUT_RXNE, true)) return 0;
			*p++ = self[IICReg::RXDR];
			xfer_size--;
			xfer_count--;
			if (xfer_size == 0 && xfer_count != 0) {
				if (!_WaitFlag(I2C_ISR_TCR, true, I2C_TIMEOUT_TCR, false)) return 0;
				xfer_size = (xfer_count > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : xfer_count;
				_TransferConfig(xfer_size, xfer_count > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : _IMM(XferMode_E::AutoEnd), 0);
			}
		}
		if (!_WaitFlag(I2C_ISR_STOPF, true, I2C_TIMEOUT_STOPF, true)) return 0;
		_FinishXfer();
		return size;
	}
	bool IIC_HARD::evByInterrupt(void) {
		if (role == IICRole::Slave) return slaveByInterrupt();
		stduint itflags = self[IICReg::ISR];
		stduint itsources = self[IICReg::CR1];
		if ((itflags & I2C_ISR_NACKF) && (itsources & I2C_CR1_NACKIE)) {
			self[IICReg::ICR] = I2C_ISR_NACKF;
			errcode |= ERR_IIC_NACK;
		}
		else if ((itflags & I2C_ISR_RXNE) && (itsources & I2C_CR1_RXIE)) {
			*rx_ptr++ = self[IICReg::RXDR];
			xfer_size--;
			xfer_count--;
		}
		else if ((itflags & I2C_ISR_TXIS) && (itsources & I2C_CR1_TXIE)) {
			self[IICReg::TXDR] = *tx_ptr++;
			xfer_size--;
			xfer_count--;
		}
		else if ((itflags & I2C_ISR_TCR) && (itsources & I2C_CR1_TCIE)) {
			if (dma_xfer) {
				// AKA I2C_Master_ISR_DMA TCR branch
				self[IICReg::CR1] &= ~_IMM(I2C_CR1_TCIE);
				if (xfer_count != 0) {
					xfer_size = (xfer_count > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : xfer_count;
					_TransferConfig(xfer_size, xfer_count > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : _IMM(XferMode_E::AutoEnd), 0);
					xfer_count -= xfer_size;
					if (self[IICReg::CR2] & I2C_CR2_RD_WRN) self[IICReg::CR1] |= I2C_CR1_RXDMAEN;
					else self[IICReg::CR1] |= I2C_CR1_TXDMAEN;
				}
			}
			else {
				// AKA I2C_Master_ISR_IT TCR branch (Rupt)
				if (xfer_size == 0 && xfer_count != 0) {
					xfer_size = (xfer_count > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : xfer_count;
					_TransferConfig(xfer_size, xfer_count > MAX_NBYTE_SIZE ? _IMM(XferMode_E::Reload) : (xfer_stop ? _IMM(XferMode_E::AutoEnd) : _IMM(XferMode_E::SoftEnd)), 0);
				}
				else errcode |= ERR_IIC_SIZE;
			}
		}
		if ((itflags & I2C_ISR_STOPF) && (itsources & I2C_CR1_STOPIE)) {
			self[IICReg::ICR] = I2C_ISR_STOPF;
			_FinishXfer();
			self[IICReg::CR1] &= ~_IMM(I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_TCIE | I2C_CR1_STOPIE);
			lock = false;
			dma_xfer = false;
			return true;
		}
		// AKA I2C_Master_ISR_IT TC branch (sequential SOFTEND frame done, no STOP generated)
		if ((itflags & I2C_ISR_TC) && (itsources & I2C_CR1_TCIE)) {
			if (xfer_count == 0) {
				self[IICReg::CR1] &= ~_IMM(I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_TCIE | I2C_CR1_STOPIE);
				lock = false;
				dma_xfer = false;
				return true;
			}
			errcode |= ERR_IIC_SIZE;
		}
		return false;
	}
	bool IIC_HARD::erByInterrupt(void) {
		stduint itflags = self[IICReg::ISR];
		stduint itsources = self[IICReg::CR1];
		bool err = false;
		if ((itflags & I2C_ISR_BERR) && (itsources & I2C_CR1_ERRIE)) {
			self[IICReg::ICR] = I2C_ISR_BERR;
			errcode |= ERR_IIC_BERR; err = true;
		}
		if ((itflags & I2C_ISR_OVR) && (itsources & I2C_CR1_ERRIE)) {
			self[IICReg::ICR] = I2C_ISR_OVR;
			errcode |= ERR_IIC_OVR; err = true;
		}
		if ((itflags & I2C_ISR_ARLO) && (itsources & I2C_CR1_ERRIE)) {
			self[IICReg::ICR] = I2C_ISR_ARLO;
			errcode |= ERR_IIC_ARLO; err = true;
		}
		return err;
	}
	// AKA HAL_I2C_Slave_Transmit_IT / HAL_I2C_Slave_Receive_IT (non-blocking)
	stduint IIC_HARD::SlaveTransmit(const byte* tx, stduint size) {
		if (!tx || !size || lock) { errcode |= ERR_IIC_SIZE; return 0; }
		lock = true;
		role = IICRole::Slave;
		tx_ptr = tx;
		xfer_count = size;
		dma_xfer = false;
		errcode = ERR_IIC_NONE;
		self[IICReg::CR2] &= ~_IMM(I2C_CR2_NACK);// enable address acknowledge
		// AKA I2C_Enable_IRQ(I2C_XFER_TX_IT | I2C_XFER_LISTEN_IT)
		self[IICReg::CR1] |= I2C_CR1_ADDRIE | I2C_CR1_TXIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_ERRIE;
		return size;
	}
	stduint IIC_HARD::SlaveReceive(byte* rx, stduint size) {
		if (!rx || !size || lock) { errcode |= ERR_IIC_SIZE; return 0; }
		lock = true;
		role = IICRole::Slave;
		rx_ptr = rx;
		xfer_count = size;
		dma_xfer = false;
		errcode = ERR_IIC_NONE;
		self[IICReg::CR2] &= ~_IMM(I2C_CR2_NACK);// enable address acknowledge
		// AKA I2C_Enable_IRQ(I2C_XFER_RX_IT | I2C_XFER_LISTEN_IT)
		self[IICReg::CR1] |= I2C_CR1_ADDRIE | I2C_CR1_RXIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_ERRIE;
		return size;
	}
	// AKA HAL I2C_Slave_ISR_IT (7-bit slave EV)
	bool IIC_HARD::slaveByInterrupt(void) {
		stduint itflags = self[IICReg::ISR];
		stduint itsources = self[IICReg::CR1];
		if ((itflags & I2C_ISR_NACKF) && (itsources & I2C_CR1_NACKIE)) {
			self[IICReg::ICR] = I2C_ISR_NACKF;
			if (xfer_count != 0) errcode |= ERR_IIC_NACK;// slave TX NACKed before completion -> error
			// xfer_count==0: master NACK after last byte is normal end, just clear the flag
		}
		else if ((itflags & I2C_ISR_ADDR) && (itsources & I2C_CR1_ADDRIE)) {
			self[IICReg::ICR] = I2C_ISR_ADDR;// AKA I2C_ITAddrCplt (7-bit: clear ADDR, DIR selects TXIS/RXNE)
		}
		else if ((itflags & I2C_ISR_RXNE) && (itsources & I2C_CR1_RXIE)) {
			if (xfer_count > 0) {
				*rx_ptr++ = self[IICReg::RXDR];
				xfer_count--;
			}
		}
		else if ((itflags & I2C_ISR_TXIS) && (itsources & I2C_CR1_TXIE)) {
			if (xfer_count > 0) {
				self[IICReg::TXDR] = *tx_ptr++;
				xfer_count--;
			}
		}
		if ((itflags & I2C_ISR_STOPF) && (itsources & I2C_CR1_STOPIE)) {
			self[IICReg::ICR] = I2C_ISR_STOPF;
			self[IICReg::CR1] &= ~_IMM(I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_ADDRIE | I2C_CR1_STOPIE);
			self[IICReg::CR2] |= I2C_CR2_NACK;// disable address acknowledge, back to listen
			lock = false;
			dma_xfer = false;
			return true;
		}
		return false;
	}
	stduint IIC_HARD::getDMARequestID(bool is_tx) const {
		return iic_dmaRequestID(id, is_tx);
	}
	void IIC_HARD::dmaTransmitCplt(void) {
		self[IICReg::CR1] &= ~_IMM(I2C_CR1_TXDMAEN);
		if (xfer_count == 0) {
			self[IICReg::CR1] |= I2C_CR1_STOPIE;
		}
		else {
			tx_ptr += xfer_size;
			xfer_size = (xfer_count > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : xfer_count;
			if (_DmaTxStart((pureptr_t)tx_ptr, (pureptr_t)(iicn_addr[id - 1] + _IMMx4(IICReg::TXDR)), xfer_size)) {
				self[IICReg::CR1] |= I2C_CR1_TCIE;
			}
			else {
				errcode |= ERR_IIC_DMA;
				self[IICReg::CR1] |= I2C_CR1_STOPIE;
			}
		}
	}
	void IIC_HARD::dmaReceiveCplt(void) {
		self[IICReg::CR1] &= ~_IMM(I2C_CR1_RXDMAEN);
		if (xfer_count == 0) {
			self[IICReg::CR1] |= I2C_CR1_STOPIE;
		}
		else {
			rx_ptr += xfer_size;
			xfer_size = (xfer_count > MAX_NBYTE_SIZE) ? MAX_NBYTE_SIZE : xfer_count;
			if (_DmaRxStart((pureptr_t)(iicn_addr[id - 1] + _IMMx4(IICReg::RXDR)), (pureptr_t)rx_ptr, xfer_size)) {
				self[IICReg::CR1] |= I2C_CR1_TCIE;
			}
			else {
				errcode |= ERR_IIC_DMA;
				self[IICReg::CR1] |= I2C_CR1_STOPIE;
			}
		}
	}
	bool IIC_HARD::IsDeviceReady(stduint dev, byte trials, stduint timeout) {
		if (lock) return false;
		errcode = ERR_IIC_NONE;
		dest_addr = dev;
		for0(t, trials) {
			if (self[IICReg::ISR] & I2C_ISR_BUSY) return false;// AKA HAL: BUSY==SET returns HAL_BUSY
			// AKA I2C_GENERATE_START (SADD + START + AUTOEND, no data)
			_TransferConfig(0, _IMM(XferMode_E::AutoEnd), _IMM(XferRequest_E::StartWrite));
			uint64 fstart = SysTick::getTick();
			while (!(self[IICReg::ISR] & I2C_ISR_STOPF) && !(self[IICReg::ISR] & I2C_ISR_NACKF)) {
				if ((SysTick::getTick() - fstart) > timeout) { errcode |= ERR_IIC_TIMEOUT; return false; }
			}
			if (self[IICReg::ISR] & I2C_ISR_NACKF) {
				self[IICReg::ICR] = I2C_ISR_NACKF;
				self[IICReg::ICR] = I2C_ISR_STOPF;
				continue;// NACK -> retry
			}
			self[IICReg::ICR] = I2C_ISR_STOPF;
			self[IICReg::CR2] &= ~_IMM(I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN);
			return true;
		}
		errcode |= ERR_IIC_NACK;
		return false;
	}


	
	#endif

#endif
}
