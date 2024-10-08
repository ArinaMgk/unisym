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

	void GeneralPurposeInputOutputPin::setMode(GPIORupt::RuptEdge edg, Handler_t fn) {
		if (fn) setInterrupt(fn);

		if (!isInput())
			setMode(GPIOMode::IN_Floating);

		#if defined(_MCU_STM32F1x)
		RCC.APB2.enAble(_RCC_APB2ENR_POSI_ENCLK_AFIO_BITPOS);

		#elif defined(_MCU_STM32F4x)
		RCC.APB2.enAble(14);// SYSCFG EN
		
		#endif
		for0(i, 10);// some delay to wait, magic_num: random
		Reference& CrtEXTICR = AFIO::ExternInterruptCfgs[bitposi >> 2];
		byte CrtPosi = (bitposi & 0x3) * 4;
		CrtEXTICR = (CrtEXTICR &
			~_IMM(0xF << CrtPosi)) |
			(GPIO.Index(parent) << CrtPosi);
		EXTI::TriggerRising.setof(bitposi, edg != GPIORupt::Negedge);
		EXTI::TriggerFalling.setof(bitposi, edg != GPIORupt::Posedge);
		EXTI::MaskInterrupt.setof(bitposi); // Mask EVENT/INTERRUPT, //{TODO}while GPIOEvent Set MaskEvent, the above are same
	}

	GeneralPurposeInputOutputPort::GeneralPurposeInputOutputPort(uint32 ADDR, uint32 CLK, uint32 Enap) :
		EnablPosi(Enap), baseaddr(ADDR), ClockPort(CLK)
	{
		for0a(i, OutpdPins) OutpdPins[i] = GeneralPurposeInputOutputPin(this, i);
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
	


	void GeneralPurposeInputOutputPin::setMode(GPIOMode::Mode mode, GPIOSpeed::Speed speed, bool autoEnClk) {
		using namespace GPIOReg;
		if (autoEnClk) parent->enClock();
		Reference ref = (bitposi < 8) ? (*parent)[CRL] : (*parent)[CRH];
		uint32 bposi = (bitposi < 8) ? bitposi << 2 : (bitposi - 8) << 2; // mul by 4
		uint32 bmode = (uint32)mode;
		bool innput = (bmode & 1);
		uint32 state = innput ? GPIOSpeed::Atmost_Input : speed;
		state |= (bmode & 0xC);// 0b1100
		ref = (ref & ~(0xf << bposi)) | (state << bposi);// or treat it a double-length Reference (64-bit)
	}


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

	void GeneralPurposeInputOutputPin::setMode(GPIOMode::Mode mode, GPIOSpeed::Speed speed, bool autoEnClk) {
		using namespace GPIOReg;
		if (autoEnClk) parent->enClock();
		(*parent)[MODER] &= ~_IMM(0x3 << (bitposi << 1));
		(*parent)[MODER] |= (_IMM(mode)>>1) << (bitposi << 1);
		BitSev((*parent)[OTYPER], bitposi, _IMM(mode) & 1);
		(*parent)[SPEED] &= ~_IMM(0x3 << (bitposi << 1));
		(*parent)[SPEED] |= _IMM(speed) << (bitposi << 1);
		if (mode == GPIOMode::IN_Floating) (*parent)[PULLS] &= ~_IMM(0x3 << (bitposi << 1));
	}

	void GeneralPurposeInputOutputPin::setPull(bool pullup) {
		using namespace GPIOReg;
		if (pullup) {
			(*parent)[PULLS] |= 0x1 << (bitposi << 1);// pull-up
			(*parent)[PULLS] &= ~_IMM(0x2 << (bitposi << 1));
		}
		else {
			(*parent)[PULLS] |= 0x2 << (bitposi << 1);// pull-dn
			(*parent)[PULLS] &= ~_IMM(0x1 << (bitposi << 1));
		}
	}

	bool GeneralPurposeInputOutputPin::isInput() const {
		using namespace GPIOReg;
		// bitposi &= 0xF;
		return 0 == (*parent)[MODER].
			mask(bitposi * 2, 2);// moder length 2 bit
	}

	void GeneralPurposeInputOutputPin::Toggle() {
		(*parent)[GPIOReg::ODR] ^= _IMM1 << bitposi;
	}

	void GeneralPurposeInputOutputPin::_set_alternate(byte selection) {
		selection &= 0xF;
		byte bposi = (bitposi & 0x7) << 2;// mod 8 then mul-by 4
		Reference r = parent->operator[](bitposi < 8 ? GPIOReg::AFRL : GPIOReg::AFRH);
		r = (r & ~_IMM(0xF << bposi)) | (selection << bposi);
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

	void GeneralPurposeInputOutputPin::setMode(GPIOMode::Mode mod, GPIOSpeed::Speed spd, bool autoEnClk) {
		using namespace GPIOReg;
		const stduint mode = mod;
		GeneralPurposeInputOutputPort& pare = getParent();
		if (autoEnClk) pare.enClock();
		pare[DIR].setof(bitposi, mode & 0x1);
		pare[OPENDRAIN].setof(bitposi, mode & 0x2);
		pare[SPEED].setof(bitposi, 0 != _IMM(spd));
		pare[ANALOG].setof(bitposi, 0);/*TEMP*/
		// innput = mode & 0x1;
	}

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
	
	GeneralPurposeInputOutputPort& GeneralPurposeInputOutputPin::getParent() const { return *parent; }
	
	GeneralPurposeInputOutputPort& GeneralPurposeInputOutput::operator[](char portid) {
		return ascii_isupper(portid) ? *(GPIO_List[
		#ifdef _MCU_MSP432P4
			portid
		#else
			portid - 'A'
		#endif
		]) : ERR;
	}

	stduint GeneralPurposeInputOutput::Index(GeneralPurposeInputOutputPort* port) {
		for0a(i, GPIO_List) if (port == GPIO_List[i])
			return i;
		return 0;
	}

	//
	GeneralPurposeInputOutputPort& GeneralPurposeInputOutputPort::operator= (uint32 val) {
		self[GPIOReg::ODR] = val;
		return *this;
	}

	// This will not be for declaration expression
	GeneralPurposeInputOutputPort& GeneralPurposeInputOutputPort::operator= (GeneralPurposeInputOutputPort& pot) {
		return self = stduint(pot);
	}

	GeneralPurposeInputOutputPort::operator stduint() {
		return _IMM(self[GPIOReg::IDR]);
	}

	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPort::operator[] (byte pinid) {
		#ifdef _MCU_STM32
		return pinid < numsof(OutpdPins) ? OutpdPins[pinid] : ERR;
		#elif defined(_MCU_CW32F030)
		return pinid < numsof(pins) ? pins[pinid] : ERR;
		#endif
	}
	
	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::operator=(const GeneralPurposeInputOutputPin& pin) {
		if (!parent) { // Initialize
			parent = pin.parent; bitposi = pin.bitposi;
			return self;
		}
		else return self = bool(pin); // Assign 
	}

	GeneralPurposeInputOutputPin::operator bool() const {
		using namespace GPIOReg;
		return getParent()[isInput() ? IDR : ODR].bitof(bitposi);
	}

	// G'DP(high-level) or D'DP(low-level)
	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::operator=(bool val) {
		getParent()[GPIOReg::ODR].setof(bitposi, val);
		return self;
	}

}

#endif
