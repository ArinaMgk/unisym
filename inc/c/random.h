// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module&Device) Pseudo-random
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
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

#ifndef _INC_Pseudo_Random
#define _INC_Pseudo_Random

#include "stdinc.h"

_ESYM_C int rand(void);
_ESYM_C void srand(unsigned int seed);

#if defined(_MPU_STM32MP13) && defined(_INC_CPP)
namespace uni {

	class RNG {
		//{TODO}
	};
	
}
#elif defined(_MCU_STM32H7x) && defined(_INC_CPP)
#include "../cpp/reference"
#include "../cpp/interrupt"
#include "../cpp/Device/SysTick"
#include "../cpp/Device/MDMA"
#include "../cpp/Device/Interrupt/interrupt_tab.h"
#include "../cpp/Device/RCC/RCCAddress"

// global ISR entry (extern "C"; defined in interrupt_rng.hpp), friended below
extern "C" void RNG_IRQHandler();

namespace uni {

	// AKA RNG_TypeDef register map (word offsets from 0x48021800)
	enum class RNGReg : unsigned {
		CR = 0x00 / 4, // 0x00: control (RNGEN/IE/CED)
		SR = 0x04 / 4, // 0x04: status (DRDY/SECS/CEIS)
		DR = 0x08 / 4, // 0x08: data register
	};

	// AKA HAL_RNG_StateTypeDef
	enum class RNGState : byte {
		Reset = 0,
		Ready,
		Busy,
		Timeout,
		Error,
	};

	// RNG_CR bits
	#define _RNG_CR_POS_RNGEN 2
	#define _RNG_CR_POS_IE    3
	#define _RNG_CR_POS_CED   5

	// RNG_SR bits
	#define _RNG_SR_POS_DRDY 0
	#define _RNG_SR_POS_SECS 2
	#define _RNG_SR_POS_CEIS 5

	// RNG hardware random generator (H7 only; AKA RNG_HandleTypeDef)
	class RNG : public RuptTrait {
	public:
		// AKA RNG_InitTypeDef.ClockErrorDetection (CR.CED); assign before setMode()
		bool clock_error_detection = false;

		// callbacks (AKA HAL_RNG_ErrorCallback / HAL_RNG_ReadyDataCallback)
		Handler_t ErrorHandler = 0;
		Handler_t ReadyDataHandler = 0;

	protected:
		// internal state (AKA tail of RNG_HandleTypeDef)
		volatile stduint RandomNumber = 0;
		volatile RNGState State = RNGState::Reset;

		// ISR entry (global, extern "C", defined in interrupt_rng.hpp)
		friend void ::RNG_IRQHandler();

	public:
		// AKA HAL_RNG_Init
		bool setMode();
		// AKA HAL_RNG_DeInit
		void canMode();
		// AKA __HAL_RNG_ENABLE / DISABLE (CR.RNGEN)
		void enAble(bool ena = true);
		// RCC AHB2ENR.RNGEN (bit 6)
		void enClock(bool ena = true);
		// AKA HAL_RNG_GenerateRandomNumber / _IT (IOMethod::Loop polls, Rupt uses IRQ)
		bool Generate(stduint& random32bit, IOMethod method = IOMethod::Loop);
		// AKA HAL_RNG_ReadLastRandomNumber
		stduint getLastRandom() const { return RandomNumber; }
		// AKA HAL_RNG_GetState
		RNGState getState() const { return State; }

		// register access
		Reference operator[](RNGReg reg) const { return Reference(0x48021800 + _IMMx4(reg)); }

		// RuptTrait (NVIC + IRQ_RNG)
		_COM_DEF_Interrupt_Interface();
	};

	extern RNG RNG1;

}

#endif

#endif
