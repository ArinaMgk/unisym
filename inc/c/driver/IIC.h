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
#include "../../cpp/Device/GPIO"

#define DA_PP 1

namespace uni {

#undef IIC
#if defined(_MCU_STM32)
	
	class IIC_t {
	protected:
		bool last_ack_accepted;
		bool push_pull;
	public:
		bool operator<<(byte txt) { Send(txt, true); return self.last_ack_accepted; }
		//
		virtual void SendStart(void) {};
		virtual void SendStop(void) {};
		virtual bool WaitAcknowledge() { return false; };
		virtual void SendAcknowledge(bool ack = true) {};
		virtual void Send(byte txt, bool auto_wait_ack = false) {};
		virtual byte ReadByte(bool feedback = true, bool ack = true) { return 0; };
	};

	class IIC_SOFT : public IIC_t {
	protected:
		GPIO_Pin& SDA, & SCL;
	public:
		Handler_t func_delay;

		IIC_SOFT(GPIO_Pin& SDA, GPIO_Pin& SCL) : SDA(SDA),
			SCL(SCL) {
			SDA.setMode(push_pull ? GPIOMode::OUT_PushPull : GPIOMode::OUT_OpenDrain);
			SCL.setMode(GPIOMode::OUT_PushPull);
			SCL = true;
			SDA = true;
		}

		virtual void SendStart(void) override;
		virtual void SendStop(void) override;
		virtual bool WaitAcknowledge() override;
		virtual void SendAcknowledge(bool ack = true) override;
		virtual void Send(byte txt, bool auto_wait_ack = false) override;
		virtual byte ReadByte(bool feedback = true, bool ack = true) override;
	};

	
	class IIC_HARD : public IIC_t {

	};
	

#endif

}

#endif
#endif
