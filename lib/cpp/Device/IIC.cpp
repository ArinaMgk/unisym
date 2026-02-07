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

namespace uni {
#if defined(_SUPPORT_GPIO)

	void IIC_SOFT::SendStart(void) {
		if (push_pull) SDA.setMode(GPIOMode::OUT_PushPull);
		SDA = true;
		SCL = true;
		asserv(func_delay)();
		SDA = false;
		asserv(func_delay)();
		SCL = false;
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

	#ifdef _MCU_STM32H7x
	IIC_HARD IIC1(1), IIC2(2), IIC3(3), IIC4(4);
	#endif

	//

	#ifdef _MCU_STM32H7x
	static const stduint iicn_addr[4] = {
		D2_APB1PERIPH_BASE + 0x5400,
		D2_APB1PERIPH_BASE + 0x5800,
		D2_APB1PERIPH_BASE + 0x5C00,
		D3_APB1PERIPH_BASE + 0x1C00
	};

	
	Reference IIC_HARD::operator[](IICReg reg) {
		return iicn_addr[id - 1] + _IMMx4(reg);
	}

	static GPIN* hSCLx[4]{ &GPIOB[6], &GPIOF[1], &GPIOH[7], &GPIOF[14] };
	static GPIN* hSDAx[4]{ &GPIOB[7], &GPIOF[0], &GPIOH[8], &GPIOF[15] };

	enum class XferMode_E : stduint { Reload = I2C_CR2_RELOAD, AutoEnd = I2C_CR2_AUTOEND, SoftEnd = 0 };
	enum class XferRequest_E : stduint {
		Stop,
		StartRead = 0x80000000U | I2C_CR2_START | I2C_CR2_RD_WRN,
		StartWrite = 0x80000000U | I2C_CR2_START,
		StartStop = 0x80000000U | I2C_CR2_STOP
	};
	
	bool IIC_HARD::enClock(bool ena) {
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
	}
	bool IIC_HARD::enAble(bool ena) {
		I2C_CR1_PE(self) = ena;
		return true;
	}

	bool IIC_HARD::setMode(stduint OwnAddress1) {
		asrtret(Ranglin(id, 1, 4)); byte _id = id - 1;

		enum class AddressingMode_E { BITS7 = 0b01, BITS10 };
		
		_TEMP _Comment("Parameters > I2C_InitTypeDef")
		stduint Timing = 0x2000090E;// Frequency range
		AddressingMode_E AddressingMode = _TEMP AddressingMode_E::BITS7;
		bool DualAddressMode = false;
		int OwnAddress2 = 0;
		int OwnAddress2Masks = (_TEMP 0) & 0b111;// 0 .. 8, 0 is none
		bool GeneralCallMode = _TEMP false;
		bool NoStretchMode = _TEMP false;
		IIC_Clksrc_E clksrc = _TEMP IIC_Clksrc_E::DxPCLK1;
		
		{
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

	void IIC_HARD::SendStart(void) {
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

	void IIC_HARD::SendAcknowledge(bool ack) { _TODO; }

	#define MAX_NBYTE_SIZE      255U
	void IIC_HARD::Send(byte* txt, stduint bitlen, bool auto_wait_ack) {
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
	byte IIC_HARD::ReadByte(bool feedback, bool ack) { _TODO  return 0; }


	
	#endif

#endif
}
