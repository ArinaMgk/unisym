// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Device) TEA5767 - FM Radio Receiver
// Codifiers: @dosconio: 20240711~;
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

#if !defined(_INC_Device_TEA5767)
#define _INC_Device_TEA5767
#include "../../stdinc.h"

#define _TEA5767_MAX_FREQ 108000
#define _TEA5767_MIN_FREQ 87500
#define _TEA5767_MAX_PLL  0x339b // 108 MHz  PLL
#define _TEA5767_MIN_PLL  0x299d // 87.5MHz  PLL
#define _TEA5767_F_IF	  225000 // 225 kHZ
#define _TEA5767_F_RES    32768 // base frequency
#define _TEA5767_ACK_WAIT_TIME	200		// IIC ack wait time 200us
#define _TEA5767_ADDR_W   (byte)0xC0
#define _TEA5767_ADDR_R   (byte)0xC1

#define _DEF_PLL ((read_buf[0] & 0x3F) << _BYTE_BITS_) | read_buf[1]

#if defined(_MCU_Intel8051)


#elif defined(_INC_CPP) // Below are C++ Area
#include "../IIC.h"

static void inline_TEA5767_delay(void) {
	for0(i, 0x1000);
}

namespace uni {
	#if defined(_MCU_STM32F10x)

	class TEA5767_t {
	protected:
		IIC_t IIC;
		bool state;
		void (*delay_ms)(stduint ms);
	public:
		byte read_buf[5], send_buf[5];
		TEA5767_t(GPIO_Pin& SDA, GPIO_Pin& SCL, void (*delay_ms)(stduint ms)) : IIC(SDA, SCL), delay_ms(delay_ms) {
			IIC.func_delay = inline_TEA5767_delay;
			send_buf[0] = 0x2E;
			send_buf[1] = 0xD6;
			send_buf[2] = 0x01;
			send_buf[3] = 0x07;
			send_buf[4] = 0x00;
			state = Send();
		}
		operator bool() { return state; }

		// true for TX
		bool setMode() {
			return state;
		}

		bool Send() {
			IIC.SendStart();
			if (IIC << _TEA5767_ADDR_W); else return false;
			for0(i, numsof(send_buf))
				if (IIC << send_buf[i]); else return false;
			IIC.SendStop();
			return true;
		}

		bool Read() {
			IIC.SendStart();
			if (IIC << _TEA5767_ADDR_R); else return false;
			for0(i, numsof(read_buf))
				read_buf[i] = IIC.ReadByte(true, i + 1 != numsof(read_buf));
			IIC.SendStop();
			return true;
		}

		uint32 getFrequency() {
			if (!Read()) return 0;
			bool HLSI = send_buf[2] & 0x10;
			uint32 PLL = _DEF_PLL;
			return (PLL * _TEA5767_F_RES / 4) + (HLSI ? -_TEA5767_F_IF : _TEA5767_F_IF);
		}
		void setFrequency(uint32 freq) {
			freq *= 1000;
			bool HLSI = send_buf[2] & 0x10;
			uint32 PLL = (float)((freq + (HLSI ? _TEA5767_F_IF : -_TEA5767_F_IF)) * 4) / _TEA5767_F_RES;// unit kHz
			PLL &= 0x3FFF;
			send_buf[0] &= 0xC0;
			send_buf[0] |= (byte)(PLL >> _BYTE_BITS_);
			send_buf[1] = (byte)PLL;
			Send();
		}

		uint32 getNextChannel(bool dir) {
			BitSev(send_buf[2], 7, dir);// 0x80
			BitSet(send_buf[0], 6);// 0x40
			delay_ms(50);
			do Read(); while (!BitGet(read_buf[0], 7));// 0x80
			return BitGet(read_buf[0], 6) ? 0 : getFrequency();// 0x40
		}

		void SearchAuto(bool mode /*???*/) {
			Read();
			bool HLSI = send_buf[2] & 0x10;
			uint32 PLL = _DEF_PLL;
			send_buf[0] = 0x40 + (PLL >> _BYTE_BITS_);
			send_buf[1] = (byte)PLL;
			send_buf[2] = mode ? 0xA0 : 0x20;
			send_buf[3] = 0x11;
			send_buf[4] = 0x00;
			Send();
			do Read(); while (!BitGet(read_buf[0], 7));// 0x80
		}
	};

	#endif
}

#endif
#endif
