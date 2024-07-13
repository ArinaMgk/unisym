// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Device) RDA5820 - FM Radio Transceiver
// Codifiers: @dosconio: 20240710~;
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

#if !defined(_INC_Device_RDA5820)
#define _INC_Device_RDA5820
#include "../../stdinc.h"

#define _RDA5820_R00   0X00
#define _RDA5820_R02   0X02
#define _RDA5820_R03   0X03
#define _RDA5820_R04   0X04
#define _RDA5820_R05   0X05
#define _RDA5820_R0A   0X0A
#define _RDA5820_R0B   0X0B
#define _RDA5820_R40   0X40
#define _RDA5820_R41   0X41
#define _RDA5820_R42   0X42
#define _RDA5820_R4A   0X4A
#define _RDA5820_R4B   0X4B
#define _RDA5820_R4C   0X4C
#define _RDA5820_R4D   0X4D
#define _RDA5820_R4E   0X4E
#define _RDA5820_R53   0X53
#define _RDA5820_R54   0X54
#define _RDA5820_R64   0X64
#define _RDA5820_READ  0X23
#define _RDA5820_WRITE 0X22

#if defined(_MCU_Intel8051)


#elif defined(_INC_CPP) // Below are C++ Area
#include "../IIC.h"

static void inline_RDA5820_delay(void) {
	for0(i, 0x1000);
}

namespace uni {
#if defined(_MCU_STM32F10x)

	class RDA5820_t {
	protected:
		IIC_t IIC;
		bool state;
		void (*delay_ms)(stduint ms);
	public:
		RDA5820_t(GPIO_Pin& SDA, GPIO_Pin& SCL, void (*delay_ms)(stduint ms)) : IIC(SDA, SCL), delay_ms(delay_ms) {
			IIC.func_delay = inline_RDA5820_delay;
			word tmp;
			if ((tmp = Read(_RDA5820_R00)) == 0x5820) // expected value
			{
				Send(_RDA5820_R02, 0x0002);// Soft Reset
				delay_ms(50);
				Send(_RDA5820_R02, 0xC001);// Stereo, power it up
				delay_ms(600);
				Send(_RDA5820_R05, 0X884F);// Search Strength 8,LNAN,1.8mA,VOL max
				Send(_RDA5820_R05, 0X884F);// Search Strength 8,LNAN,1.8mA,VOL max
				Send(0X04, 0X0400);
				Send(0X05, 0X884f);
				Send(0X14, 0X3a00);
				Send(0X15, 0X88fe);
				Send(0X16, 0X4c00);	
				Send(0X1a, 0X0400);	
				Send(0X1C, 0X23dc);	
				Send(0X27, 0Xbb6c);	
				Send(0x5C, 0x175C);	
				Send(0x68, 0x05FF);	
				Send(0x6A, 0x2846);	
				Send(0x41, 0x41FF);
				state = true;				
			}
			else state = false;
		}
		operator bool(){ return state; }

		// true for TX
		bool setMode(bool TX_or_RX, byte PGA = 0xFF, byte PAG = 0xFF) {
			if (!state) return false;
			word tmp = Read(0x40) & 0xFFF0;
			if (TX_or_RX) tmp |= 0x0001;
			Send(0x40, tmp);
			// Send Power for Tx in range 0..64
			// Input Gain for Tx in range 0..8  (?)
			if (PAG < 64)
				Send(0x42, Read(0x42) & 0xFFC0 | PAG);
			if (PGA < 8)
				Send(0x42, Read(0x42) & 0xF8FF | (PGA << 8));
			return true;
		}
		void setBand(byte band) {
			/* Band
				0: 87..108 MHz
				1: 76..91  MHz
				2: 76..108 MHz
				3: User-def (53H~54H)
			*/
			Send(0x03, Read(0x03) & 0xFFF3 | (band << 2));
		}

		void Send(byte addr, uint16 data) {
			IIC.SendStart();
			IIC << (byte)_RDA5820_WRITE;
			IIC << addr;
			IIC << (data >> _BYTE_BITS_);// High
			IIC << (data & 0XFF);// Low
			IIC.SendStop();
		}

		word Read(byte addr) {
			word res;
			IIC.SendStart();
			IIC << (byte)_RDA5820_WRITE;
			IIC << addr;
			IIC.SendStart();
			IIC << (byte)_RDA5820_READ;
			res = IIC.ReadByte(true, true) << _BYTE_BITS_;
			res |= IIC.ReadByte(true, false);
			IIC.SendStop();
			return res;
		}

		byte getRSSI() {
			word tmp = Read(0x0B);
			return (byte)(tmp >> 9); // in range 0..128 aka (0,128]
		}
		void setRSSI(byte RSSI/*0..128*/) {
			word tmp = Read(0x05) & 0x80FF | ((word)RSSI << 8);
			Send(0x05, tmp);
		}

		void setVolume(byte vol/*0..16*/) {
			vol &= 0x0F;
			word tmp = Read(0x05) & 0xFFF0 | vol;
			Send(0x05, tmp);
		}

		void setMute(bool mute = true) {
			word tmp = Read(0x02);
			if (mute) tmp |= 1 << 14;
			else tmp &= ~(1 << 14);
			Send(0x02, tmp);
		}
		void togMute() {
			Send(0x02, Read(0x02) ^ (1 << 14));
		}

		// Step Freqency: 0:100k 1:200k 2:50k 3:None
		void setSpace(byte space) {
			Send(0x03, Read(0x03) & 0xFFFC | (space & 0x03));
		}

		void /*need-delay*/ setFreqency(word freq/*unit: 10kHz */) {
			word fbtm, chan;
			word tmp = Read(0x03) & 0X001F;
			byte band = (tmp >> 2) & 0x03;
			byte spc = tmp & 0x03;
			if (spc == 0) spc = 10;
			else if (spc == 1)spc = 20;
			else spc = 5;
			if (band == 0) fbtm = 8700;
			else if (band == 1 || band == 2) fbtm = 7600;
			else fbtm = Read(0X53) * 10;// freq of bottom
			if (freq < fbtm) return;
			chan = (freq - fbtm) / spc;
			chan &= 0X3FF;
			tmp |= chan << 6;
			tmp |= 1 << 4; // TONE ENABLE			     
			Send(0X03, tmp);
			// delay and while ((RDA5820_RD_Reg(0X0B) & (1 << 7)) == 0);
		}

		word getFrequency() {
			byte spc = 0, band = 0;
			word fbtm, chan;
			word tmp = Read(0x03);
			chan = tmp >> 6;
			band = (tmp >> 2) & 0x03;
			spc = tmp & 0x03;
			if (spc == 0) spc = 10;
			else if (spc == 1) spc = 20;
			else spc = 5;
			if (band == 0) fbtm = 8700;
			else if (band == 1 || band == 2) fbtm = 7600;
			else fbtm = Read(0x53) * 10;// bottom freq
			return fbtm + chan * spc;
		}

	};

#endif
}

#endif
#endif
