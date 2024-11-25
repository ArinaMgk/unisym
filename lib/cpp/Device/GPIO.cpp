// ASCII CPP TAB4 CRLF
// Docutitle: (Device) General Purpose Input Output
// Codifiers: @dosconio: 20240412
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
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
#include "../../../inc/cpp/Device/GPIO"
#include "../../../inc/cpp/Device/RCC/RCC"
#include "../../../inc/cpp/Device/RCC/RCCAddress"
#include "../../../inc/c/binary.h"

#ifdef _SUPPORT_GPIO

// Inner Include
#if 0
#elif defined(_MCU_MSP432P4)
#include "../../../inc/c/MCU/MSP432/MSP432P4.h"
#endif

// if zero => no GPIO
static stduint _TAB_ADDR_GPIOx[] = {
#if defined(_MCU_MSP432P4)
	0,// P0
	0x40004C00, 0x40004C01,// P1 P2 (PA)
	0x40004C20, 0x40004C21,// P3 P4 (PB)
	0x40004C40, 0x40004C41,// P5 P6 (PC)
	0x40004C60, 0x40004C61,// P7 P8 (PD)
	0x40004C80, 0x40004C81,// P9 P10(PE)
	0x40004D20,// PJ
#elif defined(_MPU_STM32MP13)// PA ~ PI
	0x50002000, 0x50003000, 0x50004000, 0x50005000,
	0x50006000, 0x50007000, 0x50008000, 0x50009000, 0x5000A000,
#elif defined(_MCU_STM32F1x)// PA ~ PG
	0x40010800, 0x40010C00, 0x40011000, 0x40011400,
	0x40011800, 0x40011C00, 0x40012000,
#elif defined(_MCU_STM32F4x)// PA ~ PI
	0x40020000, 0x40020400, 0x40020800, 0x40020C00,
	0x40021000, 0x40021400, 0x40021800, 0x40021C00, 0x40022000,
#elif defined(_MCU_CW32F030)// PA
	0x48000000, 0x48000400, 0x48000800,// A B C
	0, 0, 0x48001400,// F
#else
	#error [unisym] unsupported MCU
#endif
};

namespace uni
{
	// G'DP(high-level) or D'DP(low-level)
	const GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::operator= (bool val) const {
	#ifdef _MCU_MSP432P4
		Reference_T<byte>(&BITBAND_PERI(getParent()[GPIOReg8::ODR], getID())) = byte(val);
	#else
		getParent()[GPIOReg::ODR].setof(getID(), val);
	#endif
		return self;
	}

	GeneralPurposeInputOutputPin::operator bool() const {
	#ifdef _MCU_MSP432P4
		return byte(Reference_T<byte>(&BITBAND_PERI(getParent()[GPIOReg8::IDR], getID())));
	#else
		return getParent()[GPIOReg::IDR].bitof(getID());// cancel isInput() ? IDR : ODR
	#endif
	}

	void GeneralPurposeInputOutputPin::setPull(bool pullup) const {
	#if defined(_MCU_CW32F030)
		getParent()[GPIOReg::PUR].setof(getID(), pullup);
		getParent()[GPIOReg::PDR].setof(getID(), !pullup);
	#elif defined(_MCU_STM32F1x)
		getParent()[GPIOReg::ODR].setof(getID(), pullup);// Output Directly
	#elif defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
		getParent()[GPIOReg::PULLS].maset(_IMMx2(getID()), 2, pullup ? 0b01 : 0b10);
	#endif
	}
	
	void GeneralPurposeInputOutputPin::Toggle() const
	{
	#if defined(_MCU_MSP432P4)
		Reference_T<byte> this_pin(&BITBAND_PERI(getParent()[GPIOReg8::ODR], getID()));
		this_pin = !this_pin;
	#elif defined(_MCU_CW32F030)
		getParent()[GPIOReg::TOG] = getID();
	#else
		getParent()[GPIOReg::ODR] ^= _IMM1S(getID());// no use of BSRR
	#endif
	}

	void GeneralPurposeInputOutputPin::Lock(bool tolock_orunlock) const {
		//{TODO}
	}

	bool GeneralPurposeInputOutputPin::isInput() const {
		// panic if getID() > 15
	#if defined(_MCU_CW32F030)
		return getParent()[GPIOReg::DIR].bitof(getID());
	#elif defined(_MCU_STM32F1x)
		return 0 == getParent()[getID() < 8 ? GPIOReg::CRL : GPIOReg::CRH].mask(getID() * 4, 2);
	#elif defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
		return 0 == getParent()[GPIOReg::MODER].mask(getID() * 2, 2);// moder length 2 bit
	#else
		return false;
	#endif
	}

#if defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
	// Old Style
	bool GeneralPurposeInputOutputPin::_set_alternate(byte selection) const {
	#if 1
		const stduint block_siz = 4;
		// panic if getID() > 15
		selection &= 0xF;
		byte bposi = (getID() & 0x7) * block_siz;
		getParent()[getID() < 8 ? GPIOReg::AFRL : GPIOReg::AFRH].maset(bposi, block_siz, selection);
	#endif
		return true;//{TODO} Confilict check. True for allowing
	}
#endif

	// ---- ---- ---- ---- GPORT ---- ---- ---- ----
	GeneralPurposeInputOutput GPIO;

	void GeneralPurposeInputOutputPort::enClock(bool enable) const {
	#if defined(_MPU_STM32MP13)
		using namespace RCCReg;
		RCCReg::RCCReg AHB4ENR_NS = enable ? MP_NS_AHB4ENSETR : MP_NS_AHB4ENCLRR;
		RCC[AHB4ENR_NS] = _IMM1S(getID());
	#elif defined(_MCU_CW32F030)
		Reference(_SYSC_AHBEN_ADDR).setof(_SYSC_AHBEN_POS_GPIOA + getID(), enable);
	#elif defined(_MCU_STM32F1x)
		Reference(_RCC_APB2ENR_ADDR).setof(_RCC_APB2ENR_POSI_ENCLK_GPIOA + getID(), enable);
	#elif defined(_MCU_STM32F4x)
		Reference(_RCC_AHB1ENR_ADDR).setof(_RCC_AHB1ENR_POSI_ENCLK_GPIOA + getID(), enable);
	#endif
	}
	
	#ifndef _MCU_MSP432P4 //{TEMP}
	GeneralPurposeInputOutputPort::operator stduint() const {
		return _IMM(self[GPIOReg::IDR]);
	}
	#endif
	
	#if 0
	#elif defined(_MCU_MSP432P4)
	Reference_T<byte> GeneralPurposeInputOutputPort::operator[](GPIOReg8::GPIOReg8 trt) const {
		return _TAB_ADDR_GPIOx[getID()] + _IMM(trt);
	}
	#else
	Reference GeneralPurposeInputOutputPort::operator[](GPIOReg trt) const {
		return _TAB_ADDR_GPIOx[getID()] + _IMMx4(trt);
	}
	#endif
	const GeneralPurposeInputOutputPort& GeneralPurposeInputOutputPort::operator=(stduint val) const {
	#ifdef _MCU_MSP432P4		
		ROM_GPIOTABLE[2](getID(), ~val);// set ddp
		ROM_GPIOTABLE[1](getID(),  val);// set gdp
	#else
		self[GPIOReg::ODR] = val;
	#endif
		return self;
	}

	// ---- ---- ---- ---- GPIO ---- ---- ---- ----
	
	const GeneralPurposeInputOutputPort& GeneralPurposeInputOutput::operator[](unsigned portid) const {
	#ifdef _MCU_MSP432P4
		static byte maps_caps[] = {1,3,5,7,9};//A B C D E
		return *(GeneralPurposeInputOutputPort*)(
			portid <= 10 ? (portid << 4) :
			portid == 'J' ? (11 << 4) :
			portid > 'E' ? 0xBAD :
			(portid >= '0' && portid <= '9') ? ((portid - '0') << 4) :
			ascii_isupper(portid) ? _IMM(maps_caps[portid - 'A'] << 4) : 0xBAD
			);
	#else
		return *(GeneralPurposeInputOutputPort*)_IMM((portid - 'A') << 4);
	#endif
	}

#include "../../../inc/c/driver/GPIO/GPIO-Interrupt.hpp"
	
}

#endif
