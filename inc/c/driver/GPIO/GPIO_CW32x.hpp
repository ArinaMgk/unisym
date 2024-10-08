// ASCII CPP TAB4 CRLF
// Docutitle: (Device) General Purpose Input Output, for CW32x
// Codifiers: @dosconio: 20240716
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
//! only included by inner UNISYM `.../GPIO`
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

#ifndef _INC_INN_GPIO_CW32
#define _INC_INN_GPIO_CW32
#if defined(_MCU_CW32F030)

namespace GPIOReg {
	typedef enum {
		DIR = 0, OPENDRAIN, SPEED, PDR,
		PUR, AFRH, AFRL, ANALOG,
		DRIVER, RISEIE, FALLIE, HIGHIE,
		LOWIE, ISR, ICR, LCKR,
		FILTER, REV0, REV1, REV2,
		IDR, ODR, BRR, BSRR,
		TOG
	} GPIOReg;
}
namespace GPIOMode {
	enum Mode {
		// Bit0(DIR) Bit1(OPENDRAIN)
		IN = 0x01,// DIR(1) OPENDRAIN(x)
		OUT_PushPull = 0x02,// DIR(0) OPENDRAIN(0)
		OUT_OpenDrain = 0x03// DIR(0) OPENDRAIN(1)
	};
}
namespace GPIOSpeed {
	enum Speed {
		Low = 0,
		High
	};
}

class GeneralPurposeInputOutputPin final : public RuptTrait {
	_COM_DEF_GPIO_Pin_Protected();
public:
	GeneralPurposeInputOutputPin(GeneralPurposeInputOutputPort* _parent = nullptr, byte _bitposi = 0) : parent(_parent), bitposi(_bitposi) {}
	void setPull(bool dir);
	bool getInn();

	
	_COM_DEF_GPIO_Pin_Public(GPIOSpeed::Low);
};

class GeneralPurposeInputOutputPort {
	GeneralPurposeInputOutputPin pins[16];
	GeneralPurposeInputOutputPin ERR;
	
	Reference ClockPort;
	stduint EnablPosi;// of ClockPort
	friend class GeneralPurposeInputOutputPin;
	_COM_DEF_GPIO_Port_Protected();
public:
	void enClock(bool enable = true) {
		ClockPort.setof(EnablPosi, enable);
	}
	GeneralPurposeInputOutputPort(uint32 ADDR, uint32 EnclkRef, uint32 Enapin = 0) 
	: ERR((GeneralPurposeInputOutputPort*)~0, (byte)~0), ClockPort(EnclkRef) {
		for0a(i, pins)
			pins[i] = GeneralPurposeInputOutputPin(this, byte(i));
		baseaddr = ADDR;
		EnablPosi = Enapin;
		
	}
	GeneralPurposeInputOutputPin& operator[](uint8 pinid);
	GeneralPurposeInputOutputPort& operator=(uint32 val);
	_COM_DEF_GPIO_Port_Public();
};

extern GeneralPurposeInputOutputPort GPIOA, GPIOB, GPIOC, GPIOF;

#endif
#endif
