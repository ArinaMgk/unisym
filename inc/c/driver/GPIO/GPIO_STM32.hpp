// ASCII CPP TAB4 CRLF
// Docutitle: (Device) General Purpose Input Output, for Texas Instruments MSP432
// Codifiers: @dosconio: 20241115
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

#ifndef _INC_INN_GPIO_STM32
#define _INC_INN_GPIO_STM32
#if defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)

#define defa_speed GPIOSpeed::Speed::Atmost_Medium

// Abstract Layer, should take no space.
class GeneralPurposeInputOutputPort {
public:
	uint_fast8_t getID() const { return (uint_fast8_t(this) & 0xF0) >> 4; }
	Reference operator[](GPIOReg::GPIOReg trt) const;
	const GeneralPurposeInputOutputPin& operator[](uint8 pinid) const;
	const GeneralPurposeInputOutputPort& operator=(uint16 val) const;
	//{} const GeneralPurposeInputOutputPort& GeneralPurposeInputOutputPort::operator= (GeneralPurposeInputOutputPort& pot) const;
	// const GeneralPurposeInputOutputPort::operator stduint() const;
	//{} _COM_DEF_GPIO_Port_Public();
	void enClock(bool enable = true) const;
};

#define GPIOA GPIO['A']
#define GPIOB GPIO['B']
#define GPIOC GPIO['C']
#define GPIOD GPIO['D']
#define GPIOE GPIO['E']
#define GPIOF GPIO['F']
#define GPIOG GPIO['G']
#define GPIOH GPIO['H']
#define GPIOI GPIO['I']

#endif
#endif
