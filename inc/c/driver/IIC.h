// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Protocol) Inter-Integrated Circuit, I2C
// Codifiers: @dosconio: 20240429~;
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

#if !defined(_INC_Standard_IIC)
#define _INC_Standard_IIC
#include "../stdinc.h"
#if defined(_MCU_Intel8051)

void I2C_Start(void);

void I2C_SetByte(unsigned char Byte);
unsigned char I2C_GetAck(void);

unsigned char I2C_GetByte(void);
void I2C_SetAck(unsigned char AckBit);

void I2C_Stop(void);

#elif defined(_INC_CPP) // Below are C++ Area
#include "../../cpp/reference"
#include "../../cpp/Device/GPIO"
#include "../../cpp/trait/XstreamTrait.hpp"

#define DA_PP 1

namespace uni {

#undef IIC
#if defined(_SUPPORT_GPIO)
	
	class IIC_t : public OstreamTrait, public IstreamTrait {
	protected:
		bool last_ack_accepted;
		bool push_pull = false;
	public:
		virtual int out(const char* str, stduint len) {
			for0(i, len) Send(((const byte*)str)[i], true);// do ... while ACK
			return self.last_ack_accepted;
		}
		virtual int inn() {
			return ReadByte(true, true);
		}
	public:
		bool operator<<(byte txt) { return out((char*)&txt, 1); }
		//
		virtual void SendStart(void) {}
		virtual void SendStop(void) {}
		virtual bool WaitAcknowledge() { return false; }
		virtual void SendAcknowledge(bool ack = true) {}

		//{TODO} send return ACK status

		void Send(byte txt, bool auto_wait_ack = false) { Send(&txt, _BYTE_BITS_, auto_wait_ack); }
		virtual void Send(byte* txt, stduint len, bool auto_wait_ack = false) {}
		virtual byte ReadByte(bool feedback = true, bool ack = true) { return 0; }
		//{} Read(&bits, bitlen, ...)
	};

	class IIC_SOFT : public IIC_t {
	protected:
		GPIO_Pin& SDA, & SCL;
	public:
		Handler_t func_delay;

		IIC_SOFT(GPIO_Pin& SDA, GPIO_Pin& SCL, bool init_now = true)
			: SDA(SDA), SCL(SCL) { if (init_now) setMode(); }

		void setMode() {
			SDA.setMode(push_pull ? GPIOMode::OUT_PushPull : GPIOMode::OUT_OpenDrain);
			SCL.setMode(GPIOMode::OUT_PushPull);
			SCL = true;
			SDA = true;
		}

		virtual void SendStart(void) override;
		virtual void SendStop(void) override;
		virtual bool WaitAcknowledge() override;
		virtual void SendAcknowledge(bool ack = true) override;
		virtual void Send(byte* txt, stduint len, bool auto_wait_ack = false) override;
		virtual byte ReadByte(bool feedback = true, bool ack = true) override;//[to-be-outdated]
	};

	#ifdef _MCU_STM32H7x
	#ifdef _MCU_IIC_TEMP
	#include "../../cpp/Device/_inner/IIC-STM32H7.hpp"
	#endif
	enum class IICReg // x4
	{
		CR1, CR2,
		OAR1, OAR2,
		TIMINGR,
		TIMEOUTR,
		ISR,
		ICR,
		PECR,
		RXDR, TXDR,
	};
	enum class IIC_Clksrc_E {
		DxPCLK1,// D2 for IIC123, D3 for IIC4
		PLL3, HSI, CSI
	};// for field RCC_D2CCIP2R_I2C123SEL

	#endif
	class IIC_HARD : public IIC_t {
		//
		//{TEMP} only <= 8 bit, aka BITS8
	protected:
		byte id;// 1..4 for H7
		
	public:
		// debug failed!
		// AKA HAL_I2C_Mem_Write(self, dest_addr, &txt, 1, &txt, 1, uint32_t Timeout), split into:
		virtual void SendStart(void) override;
		virtual void SendStop(void) override;
		virtual bool WaitAcknowledge() override;
		virtual void Send(byte* txt, stduint len, bool auto_wait_ack = false) override;

		//
		virtual byte ReadByte(bool feedback = true, bool ack = true) override;
		virtual void SendAcknowledge(bool ack = true) override;
		
		//
		IIC_HARD(byte _id) : id(_id) {}
		#ifdef _MCU_STM32H7x
	protected:
		stduint dest_addr = 0;
		bool send_start_just = false;
	public:
		Reference operator[] (IICReg reg);
		bool enClock(bool ena = true);
		bool enAble(bool ena = true);

		void setDestination(stduint _dest_addr) { dest_addr = _dest_addr; }
		
		// none or single address
		bool setMode(stduint self_addr0 = 0);
		#endif
	};
	#ifdef _MCU_STM32H7x
	extern IIC_HARD IIC1, IIC2, IIC3, IIC4;
	#define I2C1 IIC1
	#define I2C2 IIC2
	#define I2C3 IIC3
	#define I2C4 IIC4
	#endif


#endif

}

#endif
#endif
