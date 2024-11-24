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
	
	const GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::setMode(GPIOMode mode, GPIOSpeed speed, bool autoEnClk) const {
	#if 0
	#elif defined(_MCU_MSP432P4)
		(void)autoEnClk;//{} enClock it
	#else
		if (autoEnClk) getParent().enClock();
	#endif
	#if 0
	#elif defined(_MCU_MSP432P4)//{TEMP} ROM Method only
		(void)speed;//{}
		bool innput = GPIOMode::IN == mode ? true : false;
		ROM_GPIOTABLE[innput ? 14 : 0](getParent().getID(), _IMM1S(getID()));
	#elif defined(_MCU_CW32F030)
		const stduint mod = _IMM(mode);
		getParent()[GPIOReg::DIR].setof(getID(), mod & 0x1);
		getParent()[GPIOReg::OPENDRAIN].setof(getID(), mod & 0x2);
		getParent()[GPIOReg::SPEED].setof(getID(), 0 != _IMM(speed));
		getParent()[GPIOReg::ANALOG].setof(getID(), 0);/*TEMP*/
	#elif defined(_MCU_STM32F1x)
		uint32 bposi = (getID() < 8) ? getID() << 2 : (getID() - 8) << 2; // mul by 4
		uint32 bmode = (uint32)mode;
		bool innput = (bmode & 1);
		uint32 state = _IMM(innput ? GPIOSpeed::Atmost_Input : speed);
		state |= (bmode & 0xC);// 0b1100
		getParent()[getID() < 8 ? GPIOReg::CRL : GPIOReg::CRH].maset(bposi, 4, state);
	#elif defined(_MCU_STM32F4x) || defined(_MPU_STM32MP13)
		getParent()[GPIOReg::MODER].maset(_IMMx2(getID()), 2, _IMM(mode) >> 1);
		getParent()[GPIOReg::OTYPER].setof(getID(), _IMM(mode) & 1);
		getParent()[GPIOReg::SPEED].maset(_IMMx2(getID()), 2, _IMM(speed) >> 1);
		if (mode == GPIOMode::IN_Floating)
			getParent()[GPIOReg::PULLS].maset(_IMMx2(getID()), 2, 0);
	#endif
		return self;
	}
	

	
// Interrupt Modes
#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)

	const GeneralPurposeInputOutputPin& GeneralPurposeInputOutputPin::setMode(GPIORupt::RuptEdge edg, Handler_t fn) const {
		getParent().enClock();
		if (fn) setInterrupt(fn);
		if (!isInput()) setMode(GPIOMode::IN_Floating);

	#if defined(_MCU_STM32F1x)
		RCC.APB2.enAble(_RCC_APB2ENR_POSI_ENCLK_AFIO_BITPOS);
	#elif defined(_MCU_STM32F4x)
		RCC.APB2.enAble(14);// SYSCFG EN
	#endif
		
		for0(i, 10);// some delay to wait, magic_num: random
		Reference& CrtEXTICR = AFIO::ExternInterruptCfgs[getID() >> 2];
		CrtEXTICR.maset(_IMMx4(getID()), 4, getParent().getID());
		EXTI::TriggerRising.setof(getID(), edg != GPIORupt::Negedge);
		EXTI::TriggerFalling.setof(getID(), edg != GPIORupt::Posedge);
		EXTI::MaskInterrupt.setof(getID()); // Mask EVENT/INTERRUPT, //{TODO}while GPIOEvent Set MaskEvent, the above are same
		return self;
	}

#endif	

}
#endif
