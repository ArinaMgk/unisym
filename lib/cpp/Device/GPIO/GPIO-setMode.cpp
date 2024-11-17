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

#include "../../../../inc/cpp/Device/GPIO"
#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/Device/RCC/RCCAddress"
#include "../../../../inc/c/binary.h"

#ifdef _SUPPORT_GPIO

#if 0
#elif defined(_MCU_MSP432P4)
#include "../../../../inc/c/MCU/MSP432/MSP432P4.h"
#endif

namespace uni {

// IO Modes
#ifdef _MCU_STM32F1x
	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::setMode(GPIOMode::Mode mode, GPIOSpeed::Speed speed, bool autoEnClk) {
		using namespace GPIOReg;
		if (autoEnClk) parent->enClock();
		Reference ref = (bitposi < 8) ? (*parent)[CRL] : (*parent)[CRH];
		uint32 bposi = (bitposi < 8) ? bitposi << 2 : (bitposi - 8) << 2; // mul by 4
		uint32 bmode = (uint32)mode;
		bool innput = (bmode & 1);
		uint32 state = innput ? GPIOSpeed::Atmost_Input : speed;
		state |= (bmode & 0xC);// 0b1100
		ref = (ref & ~(0xf << bposi)) | (state << bposi);// or treat it a double-length Reference (64-bit)
		return self;
	}
#elif defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
	#if defined(_MPU_STM32MP13)
	const
	#endif
	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::setMode(GPIOMode::Mode mode, GPIOSpeed::Speed speed, bool autoEnClk)
	#if defined(_MPU_STM32MP13)
		const
	#endif
	{
		using namespace GPIOReg;
	#if defined(_MPU_STM32MP13)
		byte bitposi = getID();
	#endif
		if (autoEnClk) getParent().enClock();
		getParent()[MODER].maset(_IMMx2(bitposi), 2, _IMM(mode) >> 1);
		getParent()[OTYPER].setof(bitposi, _IMM(mode) & 1);
		getParent()[SPEED].maset(_IMMx2(bitposi), 2, _IMM(speed) >> 1);
		if (mode == GPIOMode::IN_Floating)
			getParent()[PULLS].maset(_IMMx2(bitposi), 2, 0);
		return self;
	}
#elif defined(_MCU_CW32F030)
	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::setMode(GPIOMode::Mode mod, GPIOSpeed::Speed spd, bool autoEnClk) {
		using namespace GPIOReg;
		const stduint mode = mod;
		GeneralPurposeInputOutputPort& pare = getParent();
		if (autoEnClk) pare.enClock();
		pare[DIR].setof(bitposi, mode & 0x1);
		pare[OPENDRAIN].setof(bitposi, mode & 0x2);
		pare[SPEED].setof(bitposi, 0 != _IMM(spd));
		pare[ANALOG].setof(bitposi, 0);/*TEMP*/
		// innput = mode & 0x1;
		return self;
	}
#elif defined(_MCU_MSP432P4)//{TEMP} ROM Method only
	// for single pin
	GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::setMode(GPIOMode::Mode mod, GPIOSpeed::Speed spd, bool autoEnClk) const {
		(void)autoEnClk;//{} enClock it
		(void)spd;//{}
		bool innput;
		switch (mod) {
		case GPIOMode::OUT_PushPull: case GPIOMode::OUT_OpenDrain:
			innput = false;
			break;
		case GPIOMode::IN:
			innput = true;
			break;
		default: return;
		}
		ROM_GPIOTABLE[innput ? 14 : 0](getParent().getID(), _IMM1S(getID()));
		return self;
	}
	
#endif
	
// Interrupt Modes
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)

	void GeneralPurposeInputOutputPin::setMode(GPIORupt::RuptEdge edg, Handler_t fn) {
		parent->enClock();
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

#endif	

}
#endif
