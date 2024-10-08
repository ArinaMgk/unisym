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

// HRN(OLD)STYLE for MSP432P401R Official Equipment
#define gpo(x) GPIO_PORT_P##x
#define gpi(y) GPIO_PIN##y
#define PinSet(p,xdp) (xdp? \
	GPIO_setOutputHighOnPin(p): \
	GPIO_setOutputLowOnPin (p))
#define PinGet(port,pin) (P##port##IN & BIT##pin)

class GeneralPurposeInputOutputPort {
	//GeneralPurposeInputOutputPin pins[16];
	//GeneralPurposeInputOutputPin ERR;
	//Reference ClockPort;
	//stduint EnablPosi;// of ClockPort
	//friend class GeneralPurposeInputOutputPin;
	//_COM... ...
public:
	//void enClock(bool enable = true) {
	//	ClockPort.setof(EnablPosi, enable);
	//}
	//GeneralPurposeInputOutputPort(uint32 ADDR, uint32 EnclkRef, uint32 Enapin = 0)
	//	: ERR((GeneralPurposeInputOutputPort*)~0, ~0), ClockPort(EnclkRef) {
	//	for0a(i, pins)
	//		pins[i] = GeneralPurposeInputOutputPin(this, i);
	//	baseaddr = ADDR;
	//	EnablPosi = Enapin;
	//}
	//GeneralPurposeInputOutputPin& operator[](uint8 pinid);
	//GeneralPurposeInputOutputPort& operator=(uint32 val);
	//_COM_DEF_GPIO_Port_Public();
};

//extern GeneralPurposeInputOutputPort GPIO1, GPIO2, GPIO3, GPIO4;

#endif
#endif
