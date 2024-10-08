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

	#if defined(_MCU_STM32)
		#undef IIC
	class IIC_t {
		// Dynamic for software IIC, Static for hardware IIC
	protected:
		GPIO_Pin& SDA, & SCL;
		bool last_ack_accepted;
	public:
		Handler_t func_delay;
	
		IIC_t(GPIO_Pin& SDA, GPIO_Pin& SCL) : SDA(SDA), 
			SCL(SCL) {	
			#if DA_PP == 1
				SDA.setMode(GPIOMode::OUT_PushPull);
			#else
				SDA.setMode(GPIOMode::OUT_OpenDrain);
			#endif
			SCL.setMode(GPIOMode::OUT_PushPull);
			SCL = true;
			SDA = true;
		}

		void SendStart(void) {
			#if DA_PP == 1
			SDA.setMode(GPIOMode::OUT_PushPull);
			#endif
			SDA = true;
			SCL = true;
			asserv(func_delay)();
			SDA = false;
			asserv(func_delay)();
			SCL = false;
		}
		
		void SendStop(void) {
			#if DA_PP == 1
			SDA.setMode(GPIOMode::OUT_PushPull);
			#endif
			SCL = false;// opt?
			SDA = false;
			asserv(func_delay)();
			SCL = true;
			SDA = true;
			asserv(func_delay)();
		}
		
		bool WaitAcknowledge() {
			#if DA_PP == 1
			SDA.setMode(GPIOMode::IN_Floating);
			#endif
			byte timespan = 0;
			SDA = true;
			asserv(func_delay)();
			SCL = true;
			asserv(func_delay)();
			while (SDA) {
				if (++timespan) {
					SendStop();
					return last_ack_accepted = false;
				}
				asserv(func_delay)();
			}
			SCL = false;
			return last_ack_accepted = true;
		}
		
		void SendAcknowledge(bool ack = true) {
			#if DA_PP == 1
			SDA.setMode(GPIOMode::OUT_PushPull);
			#endif
			SCL = false;
			SDA = !ack;
			asserv(func_delay)();
			SCL = true;
			asserv(func_delay)();
			SCL = false;
		}
		
		void Send(byte txt, bool auto_wait_ack = false) {
			#if DA_PP == 1
			SDA.setMode(GPIOMode::OUT_PushPull);
			#endif
			SCL = false;
			for0(i, _BYTE_BITS_) {
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
		bool operator<<(byte txt) { Send(txt, true); return self.last_ack_accepted; }
		
		byte ReadByte(bool feedback = true, bool ack = true) {
			#if DA_PP == 1
			SDA.setMode(GPIOMode::IN_Floating);
			#endif
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
	};

	#endif

}

#endif
#endif
