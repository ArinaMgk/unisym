// ASCII CPL TAB4 CRLF
// Docutitle: (Module) Time and Clock
// Codifiers: @dosconio: 20240220 ~ 20240429
// Attribute: Arn-Covenant Any-Architect Bit-32mode <Reference/Dependence>
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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


#ifndef _INC_DEVICE_Timer
#define _INC_DEVICE_Timer

// [Category]
#include "RealtimeClock.h"// RTC
// PIT
// TSC

#ifdef _INC_CPP
#include "../../cpp/interrupt"
#endif

#if defined(_MCCA)

#if (_MCCA & 0xFF00) == 0x1000

#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ // interval ~= 1s

namespace uni {
	enum class MSIP_Type : byte {
		AckRupt = 0,
		SofRupt = 1,
	};
	class Timer : public RuptTrait
	{
	public:
		virtual void setInterrupt(Handler_t _func) const override {}
		virtual void setInterruptPriority(byte preempt, byte sub_priority) const override {}
		virtual void enInterrupt(bool enable = true) const override;
	public:
		uint64 Read();
		void MSIP(stduint mhartid, MSIP_Type);
		void Load(stduint mhartid, uint64 timepoint);
	protected:

	};
}
extern uni::Timer clint;
#endif

#elif defined(_MCU_Intel8051)
extern void (*RoutineINT0)(void);// Keil will consider this as a define but a declaration? Haruno RFR19.
void Timer0Init(void);
void Timer0SetCounter(unsigned int Value);
unsigned int Timer0GetCounter(void);
void Timer0Mode(byte Flag);// para0: 0 is stop, 1 is run

/* Example
void RINT0(void)
{
	//
} void (*RoutineINT0)(void) = RINT0;
*/

#endif

#endif
