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

#if defined(_SUPPORT_GPIO)
#undef SPI
	class SPI_t {
	// Dynamic for software, Static for hardware
	protected:
		// bool CPOL; when SCLK==CPOL, Trans is idle
		// bool CPHA; sample at first edge and send secondly if zero, or second edge
		virtual byte Transceive(byte data) { return data & nil; }
	public:
		Handler_t func_delay;

		virtual byte Transceivex(stduint data, byte blen) { return nil; }
		virtual void SendStart() {}
		virtual void SendStop() {}
		//
		void Send(byte data) { (void)Transceive(data); }
		// 0xFF is a dummy data
		byte Read(void) { return Transceive(0xFF); }
		SPI_t& operator<<(byte txt) { Send(txt); return self; }
		void operator>>(byte& txt) { txt = Read(); }
	};

	class SPI_SOFT : public SPI_t {
	protected:
	protected:
		GPIO_Pin& SCLK,
			& MOSI,
			& MISO,
			& CSEL;// Chip select

		virtual byte Transceive(byte data) override;
	public:
		SPI_SOFT(GPIO_Pin& SCLK, GPIO_Pin& MOSI, GPIO_Pin& MISO, GPIO_Pin& CSEL);
		virtual byte Transceivex(stduint data, byte blen) override;
		virtual void SendStart() override;
		virtual void SendStop() override;
	};
	class SPI_HARD : public SPI_t {

	};//{TODO}

#endif

}

#endif
#endif
