// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) AD9959
// Codifiers: @dosconio: 20240731 ~ <Last-check>
// Attribute:
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

//{TODO} with SPI ?

#ifndef _INC_Device_AD9959
#define _INC_Device_AD9959

#include "../../stdinc.h"
#include "../SPI.h"

#define _AD9959_CSR   0x00   //CSR   Channel select register
#define _AD9959_FR1   0x01   //FR1   Function register 1
#define _AD9959_FR2   0x02   //FR2   Function register 2
#define _AD9959_CFR   0x03   //CFR   Channel function register
#define _AD9959_CFTW0 0x04   //CTW0  Channel frequency conversion word register
#define _AD9959_CPOW0 0x05   //CPW0  Channel phase conversion word register
#define _AD9959_ACR   0x06   //ACR   Amplitude control register
#define _AD9959_LSRR  0x07   //LSR   Channel linear scan register
#define _AD9959_RDW   0x08   //RDW   Channel linear up-scan register
#define _AD9959_FDW   0x09   //FDW   Channel linear down-scan register

namespace uni {
	class AD9959 {
	protected:
		GPIO_Pin& RST, & IOUP;
		SPI_t spi;
		//
		byte CSR_DATA0[1];
		byte CSR_DATA1[1];
		byte CSR_DATA2[1];
		byte CSR_DATA3[1];
		byte FR1_DATA[3];
		byte FR2_DATA[2];
		byte CFR_DATA[3];
		byte CPOW0_DATA[2];
		byte LSRR_DATA[2];
		byte RDW_DATA[4];
		byte FDW_DATA[4];
		byte ACRdata[3];
		//
		byte* getCSR_DATAx(byte chan) {
			switch (chan) {
			case 0: return CSR_DATA0;
			case 1: return CSR_DATA1;
			case 2: return CSR_DATA2;
			case 3: return CSR_DATA3;
			default: return nullptr;
			}
		}

		void Reset(void) {
			RST = 0;
			asserv(func_delay)();
			RST.Toggle();
			asserv(func_delay)();
			RST.Toggle();
		}

		void Update() {
			IOUP = 0;
			asserv(func_delay)();
			IOUP.Toggle();
			asserv(func_delay)();
			IOUP.Toggle();
		}
	public:
		Handler_t func_delay;
		AD9959(GPIO_Pin& RST, GPIO_Pin& IOUP, GPIO_Pin& SDIO, GPIO_Pin& SCLK, GPIO_Pin& CS, Handler_t func_delay) : RST(RST), IOUP(IOUP), spi(SCLK, SDIO, SDIO, CS) {
			CSR_DATA0[0] = 0x10; // enable CH0
			CSR_DATA1[0] = 0x20; // enable CH1
			CSR_DATA2[0] = 0x40; // enable CH2
			CSR_DATA3[0] = 0x80; // enable CH3
			FR1_DATA[0] = 0xD0, FR1_DATA[1] = 0x00, FR1_DATA[2] = 0x00; // 20 times freq
			FR2_DATA[0] = 0x00, FR2_DATA[1] = 0x00; // default value 0x0000
			CFR_DATA[0] = 0x00, CFR_DATA[1] = 0x03, CFR_DATA[2] = 0x02; // default value 0x000302
			CPOW0_DATA[0] = 0x00, CPOW0_DATA[1] = 0x00; // default value 0x0000  which equals POW/2^14*360
			LSRR_DATA[0] = 0x00, LSRR_DATA[1] = 0x00;
			RDW_DATA[0] = 0x00, RDW_DATA[1] = 0x00, RDW_DATA[2] = 0x00, RDW_DATA[3] = 0x00;
			FDW_DATA[0] = 0x00, FDW_DATA[1] = 0x00, FDW_DATA[2] = 0x00, FDW_DATA[3] = 0x00;
			ACRdata[0] = 0x00, ACRdata[1] = 0x10, ACRdata[2] = 0x00;// default value 0x--0000 Rest = 18.91/Iout
			spi.func_delay = func_delay;
			self.func_delay = func_delay;
			RST.setMode(GPIOMode::OUT_PushPull);
			IOUP.setMode(GPIOMode::OUT_PushPull);
			IOUP = 0;
			Reset();
			Send(_AD9959_FR1, FR1_DATA, 3, true);
		}

		void setMode() {}
		
		void Send(byte regaddr, const byte* srcdata, byte srclens, bool update) {
			spi.SendStart();// SCLK=0, CS=0
			spi << regaddr;
			for0(i, srclens) 
				spi << srcdata[i];
			if (update) Update();
			spi.SendStop();// CS=1
		}

		void setFrequency(byte chan, uint32 freq) {
			if (chan > 4) return;
			MIN(freq, 200000000);
			byte CFTW0_DATA[4] = { 0x00,0x00,0x00,0x00 };
			uint32 freq_temp = freq * 8.589934592;
			CFTW0_DATA[3] = freq_temp;
			CFTW0_DATA[2] = (freq_temp >> 8);
			CFTW0_DATA[1] = (freq_temp >> 16);
			CFTW0_DATA[0] = (freq_temp >> 24);
			Send(_AD9959_CSR, getCSR_DATAx(chan), 1, true);
			Send(_AD9959_CFTW0, CFTW0_DATA, 4, true);
		}

		// 10-bit 0~1023 (->0 ~ 530mV)
		void setAmplitude(byte chan, float _ampl) {
			uint16 ampl = _ampl * 50.0 / 23; // dosconio expi : 23mV for 50
			MIN(ampl, 1023);
			if (chan > 4) return;
			byte ACR_DATA[3] = { 0x00,0x00,0x00 };
			ampl |= 0x1000;
			ACR_DATA[2] = ampl;
			ACR_DATA[1] = (ampl >> 8);
			Send(_AD9959_CSR, getCSR_DATAx(chan), 1, true);
			Send(_AD9959_ACR, ACR_DATA, 3, true);
		}

		void setPhase(byte chan,  float _phaz/* 14b 0~16383 0..360*/) {
			if (chan > 4) return;
			uint16 phaz = _phaz * 45.511111; // 45.511111 zo 2^14/360
			CPOW0_DATA[1] = phaz;
			CPOW0_DATA[0] = (phaz >> 8);
			Send(_AD9959_CSR, getCSR_DATAx(chan), 1, true);
			Send(_AD9959_CPOW0, CPOW0_DATA, 2, true);
		}

	};

}

#endif
