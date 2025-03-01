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

	byte SPI_SOFT::Transceivex(stduint data, byte blen) {
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


#endif
}


