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

#if !defined(_INC_Standard_SPI)
#define _INC_Standard_SPI
#include "../stdinc.h"
#if defined(_MCU_Intel8051)


#elif defined(_INC_CPP) // Below are C++ Area
#include "../../cpp/Device/GPIO"

#define DA_PP 1

namespace uni {

#if defined(_MCU_STM32)
#undef SPI
	class SPI_t {
	// Dynamic for software, Static for hardware
	protected:
		// bool CPOL; when SCLK==CPOL, Trans is idle
		// bool CPHA; sample at first edge and send secondly if zero, or second edge
		GPIO_Pin& SCLK,
			& MOSI,
			& MISO,
			& CSEL;// Chip select

		byte Transceive(byte data) {
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
				if (MISO.getAddress() != MOSI.getAddress() && MISO) res |= 1;
			}
			SCLK = 0;
			return res;
		}
	public:
		Handler_t func_delay;
		SPI_t(GPIO_Pin& SCLK, GPIO_Pin& MOSI, GPIO_Pin& MISO, GPIO_Pin& CSEL) : SCLK(SCLK), MOSI(MOSI), MISO(MISO), CSEL(CSEL)
		{
			SCLK.setMode(GPIOMode::OUT_PushPull);
			if (MISO.getAddress() != MOSI.getAddress())
				MISO.setMode(GPIOMode::IN_Floating);
			MOSI.setMode(GPIOMode::OUT_PushPull);
			CSEL.setMode(GPIOMode::OUT_PushPull);
			CSEL = 1;
			SCLK = 0;
			MOSI = _TEMP 0;
		}

		byte Transceivex(stduint data, byte blen) {
			byte res = 0;// rx
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
				if (MISO.getAddress() != MOSI.getAddress() && MISO) res |= 1;
			}
			SCLK = 0;
			return res;
		}
		
		void Send(byte data) {
		    (void)Transceive(data);
		}

		byte Read(void) {
			return Transceive(0xFF);// 0xFF is a dummy data
		}

		SPI_t& operator<<(byte txt) {
			Send(txt);
			return self;
		}

		void operator>>(byte& txt) {
			txt = Read();
		}

		void SendStart() {
			CSEL = 1;
			SCLK = 0;
			MOSI = 0;
			CSEL.Toggle();
		}

		void SendStop() {
			MOSI = 0;
			CSEL = 1;
		}
	};

#endif

}

#endif
#endif
