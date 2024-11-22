// ASCII CPP TAB4 CRLF
// Docutitle: (Device) General Purpose Input Output, for Texas Instruments MSP432
// Codifiers: @dosconio: 20240803
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

// info: The Pin / Bit can be reflected like a byte, aka bitfield
//{TODO} write this for STC89C5x, by ASM

#ifndef _INC_INN_GPIO_MSP432
#define _INC_INN_GPIO_MSP432
#if defined(_MCU_MSP432P4)

// Supple for Official

#define PinSet(p,xdp) (xdp? \
	GPIO_setOutputHighOnPin(p): \
	GPIO_setOutputLowOnPin (p))
#define PinGet(port,pin) (P##port##IN & BIT##pin)

// [MAP] 20241017 dosconio virtual-pin-softmap
// Offset 0001_000x ~ 0001_00Ax -> Port 0~10
// Offset 0001_00x0 ~ 0001_00xF -> Pin  0~15
// 0x0000_0BAD for failure

namespace GPIOReg8 {
	typedef enum {
		IDR = 0x00, // Input Data Register
		ODR = 0x02, // Output Data Register
		DIR = 0x04, // Direction Register
		REN = 0x06, // Resistor Enable Register
		DS = 0x08, // Drive Strenght Register
		SEL0 = 0x0A, // Select Register 0
		SEL1 = 0x0C, // Select Register 1
		// IV = 0x0E, // (WORD) Interrupt Vector Register
		//
		SELC = 0x16, // Complement Register
		IES = 0x18, // Interrupt Edge Select Register
		IE = 0x1A, // Interrupt Enable Register
		IFG = 0x1C // Interrupt Flag Register
	} GPIOReg8;// LEN BYTE only
}// GPIOReg for 16-bit
namespace GPIOMode {
	enum Mode {
		//{TODO}
		IN,
		OUT_PushPull,
		OUT_OpenDrain
	};
}
namespace GPIOSpeed {
	enum Speed {
		_TODO TODO
	};
}
#define defa_speed GPIOSpeed::Speed::TODO

// Abstract Layer, should take no space.
class GeneralPurposeInputOutputPort {
public:
	uint_fast8_t getID() const { return (uint_fast8_t(this) & 0xF0) >> 4; }
	Reference_T<byte> operator[](GPIOReg8::GPIOReg8 trt) const;
	const GeneralPurposeInputOutputPin& operator[](uint8 pinid) const;
	const GeneralPurposeInputOutputPort& operator=(uint16 val) const;
	//{} const GeneralPurposeInputOutputPort& GeneralPurposeInputOutputPort::operator= (GeneralPurposeInputOutputPort& pot) const;
	// const GeneralPurposeInputOutputPort::operator stduint() const;
	//{} _COM_DEF_GPIO_Port_Public();
};

//extern GeneralPurposeInputOutputPort GPIO1, GPIO2, GPIO3, GPIO4;

#endif
#endif
