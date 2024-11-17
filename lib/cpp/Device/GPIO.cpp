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

#if 0
#elif defined(_MCU_MSP432P4)
#include "../../../inc/c/MCU/MSP432/MSP432P4.h"
#endif

static stduint _TAB_ADDR_GPIOx[] = {
#if defined(_MCU_MSP432P4)
	0,// P0
	0x40004C00,// P1(A LOW)
	0x40004C01,// P2(A HIG)
	0x40004C20,// P3(B LOW)
	0x40004C21,// P4(B HIG)
	0x40004C40,// P5(C LOW)
	0x40004C41,// P6(C HIG)
	0x40004C60,// P7(D LOW)
	0x40004C61,// P8(D HIG)
	0x40004C80,// P9(E LOW)
	0x40004C81,// P10
	0x40004D20,// PJ
#elif defined(_MPU_STM32MP13)
	0x50002000,// PA
	0x50003000,// PB
	0x50004000,// PC
	0x50005000,// PD
	0x50006000,// PE
	0x50007000,// PF
	0x50008000,// PG
	0x50009000,// PH
	0x5000A000,// PI
#else
	nil
#endif
};

namespace uni
{

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)

	static Request_t GPIO_Request_list[16] = {
		IRQ_EXTI0, IRQ_EXTI1, IRQ_EXTI2, IRQ_EXTI3,
		IRQ_EXTI4, IRQ_EXTI9_5, IRQ_EXTI9_5, IRQ_EXTI9_5,
		IRQ_EXTI9_5, IRQ_EXTI9_5, IRQ_EXTI15_10, IRQ_EXTI15_10,
		IRQ_EXTI15_10, IRQ_EXTI15_10, IRQ_EXTI15_10, IRQ_EXTI15_10
	};

	void GeneralPurposeInputOutputPin::setInterrupt(Handler_t fn) {
		FUNC_EXTI[bitposi] = fn;
	}


	void GeneralPurposeInputOutputPin::setInterruptPriority(byte preempt, byte sub_priority) {
		NVIC.setPriority(GPIO_Request_list[bitposi], preempt, sub_priority);
	}

	//{TODO}
	void GeneralPurposeInputOutputPin::enInterrupt(bool enable) {
		if (bitposi < numsof(GPIO_Request_list))
			NVIC.setAble(GPIO_Request_list[bitposi], enable);
	}

	GeneralPurposeInputOutputPort::GeneralPurposeInputOutputPort(uint32 ADDR, uint32 CLK, uint32 Enap) :
		EnablPosi(Enap), baseaddr(ADDR), ClockPort(CLK)
	{
		for0a(i, OutpdPins) OutpdPins[i] = GeneralPurposeInputOutputPin(this, i);
	}

#endif
#if defined(_MPU_STM32MP13) || defined(_MCU_STM32F4x)
	void GeneralPurposeInputOutputPin::Toggle()
	#if defined(_MPU_STM32MP13)
		const
	#endif
	{
		getParent()[GPIOReg::ODR] ^= _IMM1S(getID());// no use of BSRR
	}
#endif
#ifdef _MCU_STM32F1x

	GeneralPurposeInputOutputPort GPIOA(0x40010800, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOA);
	GeneralPurposeInputOutputPort GPIOB(0x40010C00, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOB);
	GeneralPurposeInputOutputPort GPIOC(0x40011000, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOC);
	GeneralPurposeInputOutputPort GPIOD(0x40011400, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOD);
	GeneralPurposeInputOutputPort GPIOE(0x40011800, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOE);
	GeneralPurposeInputOutputPort GPIOF(0x40011C00, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOF);
	GeneralPurposeInputOutputPort GPIOG(0x40012000, _RCC_APB2ENR_ADDR, _RCC_APB2ENR_POSI_ENCLK_GPIOG);

	static GeneralPurposeInputOutputPort* GPIO_List[] = {
	&GPIOA, &GPIOB, &GPIOC, &GPIOD, &GPIOE, &GPIOF, &GPIOG
	};

#if 1
	
	// for F1, only for input (?)
	void GeneralPurposeInputOutputPin::setPull(bool pullup) {
		(*parent)[GPIOReg::ODR].setof(bitposi, pullup);
	}

	bool GeneralPurposeInputOutputPin::isInput() const {
		using namespace GPIOReg;
		// bitposi &= 0x7;
		return 0 == (*parent)[bitposi < 8 ? CRL : CRH].
			mask(bitposi * 4, 2);
	}
	
	void GeneralPurposeInputOutputPin::Toggle() {
		(*parent)[GPIOReg::ODR] ^= _IMM1 << bitposi;
	}

	//{unchecked}
	void Lock(bool tolock_orunlock) {
		//{TODO}
	}
#endif
// ---- ---- ---- ----
#elif defined(_MCU_STM32F4x)

	GeneralPurposeInputOutputPort GPIOA(0x40020000, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOA);// ~ 0x400203FF
	GeneralPurposeInputOutputPort GPIOB(0x40020400, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOB);
	GeneralPurposeInputOutputPort GPIOC(0x40020800, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOC);
	GeneralPurposeInputOutputPort GPIOD(0x40020C00, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOD);//
	GeneralPurposeInputOutputPort GPIOE(0x40021000, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOE);
	GeneralPurposeInputOutputPort GPIOF(0x40021400, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOF);
	GeneralPurposeInputOutputPort GPIOG(0x40021800, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOG);
	GeneralPurposeInputOutputPort GPIOH(0x40021C00, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOH);
	GeneralPurposeInputOutputPort GPIOI(0x40022000, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOI);
	// GeneralPurposeInputOutputPort GPIOJ(0x40022400, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOJ);
	// GeneralPurposeInputOutputPort GPIOK(0x40022800, _RCC_AHB1ENR_ADDR, _RCC_AHB1ENR_POSI_ENCLK_GPIOK);

	static GeneralPurposeInputOutputPort* GPIO_List[] = {
		&GPIOA, &GPIOB, &GPIOC, &GPIOD, &GPIOE, &GPIOF, &GPIOG, &GPIOH, &GPIOI
	};

	void GeneralPurposeInputOutputPin::setPull(bool pullup) {
		using namespace GPIOReg;
		if (pullup) {
			(*parent)[PULLS] |= 0x1 << (bitposi << 1);// pull-up
			(*parent)[PULLS] &= ~_IMM(0x2 << (bitposi << 1));
		}
		else {
			(*parent)[PULLS] |= 0x2 << (bitposi << 1);// pull-dn
			(*parent)[PULLS] &= ~_IMM1S(bitposi << 1);
		}
	}

	bool GeneralPurposeInputOutputPin::isInput() const {
		using namespace GPIOReg;
		// panic if bitposi > 15
		return 0 == (*parent)[MODER].
			mask(bitposi * 2, 2);// moder length 2 bit
	}

	bool GeneralPurposeInputOutputPin::_set_alternate(byte selection) {
		using namespace GPIOReg;
		const stduint block_siz = 4;
		// panic if bitposi > 15
		selection &= 0xF;
		byte bposi = (bitposi & 0x7) * block_siz;
		Reference r = parent->operator[](bitposi < 8 ? AFRL : AFRH);
		r.maset(bposi, block_siz, selection);
		return true;//{TODO} Confilict check. True for allowing
	}
#elif defined(_MPU_STM32MP13)
	void GeneralPurposeInputOutputPort::enClock(bool enable) const {
		using namespace RCCReg;
		RCCReg::RCCReg AHB4ENR_NS = enable ? MP_NS_AHB4ENSETR : MP_NS_AHB4ENCLRR;
		RCC[AHB4ENR_NS].setof(getID());
	}
// ---- ---- ---- ----
#elif defined(_MCU_CW32F030)

	GeneralPurposeInputOutputPort GPIOA(0x48000000, _SYSC_AHBEN_ADDR, _SYSC_AHBEN_POS_GPIOA);
	GeneralPurposeInputOutputPort GPIOB(0x48000400, _SYSC_AHBEN_ADDR, _SYSC_AHBEN_POS_GPIOB);
	GeneralPurposeInputOutputPort GPIOC(0x48000800, _SYSC_AHBEN_ADDR, _SYSC_AHBEN_POS_GPIOC);
	GeneralPurposeInputOutputPort GPIOF(0x48001400, _SYSC_AHBEN_ADDR, _SYSC_AHBEN_POS_GPIOF);

	static GeneralPurposeInputOutputPort* GPIO_List[] = {
		&GPIOA, &GPIOB, &GPIOC, nullptr, nullptr, &GPIOF
	};
	
	bool GeneralPurposeInputOutputPin::isInput() const {
		using namespace GPIOReg;
		return getParent()[DIR].bitof(bitposi);
	}

	void GeneralPurposeInputOutputPin::Toggle() {
		using namespace GPIOReg;
		GeneralPurposeInputOutputPort& pare = getParent();
		pare[TOG].setof(bitposi);
	}

	void GeneralPurposeInputOutputPin::setPull(bool dir) {
		using namespace GPIOReg;
		GeneralPurposeInputOutputPort& pare = getParent();
		pare[PUR].setof(bitposi, dir);
		pare[PDR].setof(bitposi, !dir);
	}

	void GeneralPurposeInputOutputPin::setInterrupt(Handler_t fn) { _TODO(void) fn; }
	void GeneralPurposeInputOutputPin::setInterruptPriority(byte preempt, byte sub_priority) { _TODO(void) preempt; (void)sub_priority; }
	void GeneralPurposeInputOutputPin::enInterrupt(bool enable) { _TODO(void) enable; }


#endif

	GeneralPurposeInputOutput GPIO;
	
	GeneralPurposeInputOutputPort& GeneralPurposeInputOutput::operator[](byte portid) {
	#ifdef _MCU_MSP432P4
		static byte maps_caps[] = {1,3,5,7,9};//A B C D E
		return *(GeneralPurposeInputOutputPort*)(
			portid <= 10 ? (portid << 4) :
			portid == 'J' ? (11 << 4) :
			portid > 'E' ? 0xBAD :
			(portid >= '0' && portid <= '9') ? ((portid - '0') << 4) :
			ascii_isupper(portid) ? (maps_caps[portid - 'A'] << 4) : 0xBAD
			);
	#elif defined(_MCU_CW32F030) || defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
		return ascii_isupper(portid) ? *(GPIO_List[portid - 'A']) : ERR;
	#elif defined(_MPU_STM32MP13)
		return *(GeneralPurposeInputOutputPort*)_IMM((portid - 'A') << 4);
	#endif
	}

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_CW32F030) //{} OLD Style
	
	GeneralPurposeInputOutputPort& GeneralPurposeInputOutputPin::getParent() const { return *parent; }
	

	stduint GeneralPurposeInputOutput::Index(GeneralPurposeInputOutputPort* port) {
		for0a(i, GPIO_List) if (port == GPIO_List[i])
			return i;
		return 0;
	}

	// This will not be for declaration expression
	GeneralPurposeInputOutputPort& GeneralPurposeInputOutputPort::operator= (GeneralPurposeInputOutputPort& pot) {
		return self = stduint(pot);
	}

	GeneralPurposeInputOutputPort::operator stduint() {
		return _IMM(self[GPIOReg::IDR]);
	}

	
	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::operator=(const GeneralPurposeInputOutputPin& pin) {
		if (!parent) { // Initialize
			parent = pin.parent; bitposi = pin.bitposi;
			return self;
		}
		else return self = bool(pin); // Assign 
	}


#elif defined(_MCU_MSP432P4)
	
	const GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::operator=(const GeneralPurposeInputOutputPin& pin) const{
		self = bool(pin);
		return self;
	}



	void GeneralPurposeInputOutputPin::Toggle() const {
		Reference_T<byte> this_pin(&BITBAND_PERI(getParent()[GPIOReg8::ODR], getID()));
		this_pin = !this_pin;
	}

	Reference_T<byte> GeneralPurposeInputOutputPort::operator[](GPIOReg8::GPIOReg8 trt) const {
		return _TAB_ADDR_GPIOx[getID()] + _IMM(trt);
	}
#elif defined(_MPU_STM32MP13)
	Reference GeneralPurposeInputOutputPort::operator[](GPIOReg::GPIOReg trt) const {
		return _TAB_ADDR_GPIOx[getID()] + _IMMx4(trt);
	}
#endif

#ifndef _MPU_STM32MP13 //{TEMP}
	
	//{} Combine GeneralPurposeInputOutputPin::operator= here
	
	//
	#ifdef _MCU_MSP432P4
	const
	#endif
	GeneralPurposeInputOutputPort& GeneralPurposeInputOutputPort::operator=
	#ifdef _MCU_MSP432P4
		(uint16 val) const {
		ROM_GPIOTABLE[2](getID(), ~val);// set ddp
		ROM_GPIOTABLE[1](getID(),  val);// set gdp
		//{TODO} Try by _TAB_ADDR_GPIOx_OUT
		return self;
	}
	#else
		(uint32 val) {
		using namespace GPIOReg; self[ODR] = val;
		return self;
	}
	#endif
	
	GeneralPurposeInputOutputPin::operator bool() const {
		#ifdef _MCU_MSP432P4
		return byte(Reference_T<byte>(&BITBAND_PERI(getParent()[GPIOReg8::ODR], getID())));
		#else
		using namespace GPIOReg;
		return getParent()[isInput() ? IDR : ODR].bitof(bitposi);
		#endif
	}

	// G'DP(high-level) or D'DP(low-level)
	#ifdef _MCU_MSP432P4
	const
	#endif
	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::operator= (bool val)
	#ifdef _MCU_MSP432P4
		const
	#endif
	{
	#ifdef _MCU_MSP432P4
		Reference_T<byte>(&BITBAND_PERI(getParent()[GPIOReg8::ODR], getID())) = byte(val);
	#else
		getParent()[GPIOReg::ODR].setof(bitposi, val);
	#endif
		return self;
	}
#endif

	#if !defined(_MCU_STM32F1x) && !defined(_MCU_STM32F4x) && !defined(_MCU_CW32F030)
	const
	#endif
	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPort::operator[] (byte pinid)
	#if !defined(_MCU_STM32F1x) && !defined(_MCU_STM32F4x) && !defined(_MCU_CW32F030)
		const
	#endif
	{
		#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
		return pinid < numsof(OutpdPins) ? OutpdPins[pinid] : ERR;
		#elif defined(_MCU_CW32F030)
		return pinid < numsof(pins) ? pins[pinid] : ERR;
		#else // New Style
		return *(const GeneralPurposeInputOutputPin*)((_IMM(this) & 0xF0) + (pinid & 0xF));
		#endif
	}
	

}

#endif
