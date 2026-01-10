// ASCII CPL TAB4 CRLF
// Docutitle: (Device) Programmable Interval Timer
// Codifiers: @dosconio: 20240529
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
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

#include "../../../../inc/c/driver/i8259A.h"
#include "../../../../inc/c/driver/PIT.h"

#if defined(_MCCA) && (_MCCA==0x8616||_MCCA==0x8632)

#define RATE_GENERATOR 0x34 // 00-11-010-0 : {Counter0 - LSB then MSB - rate generator - binary}
#define TIMER_FREQ     1193182L // clock frequency for timer in PC and AT */
#define kHZ             1000  // clock freq (software settable on IBM-PC) */

#define BEEP_HZ 440
#define BEEP_COUNTER (TIMER_FREQ / BEEP_HZ)

void PIT_Init()
{
	// 1kHz
	outpb(PORT_TIMER_MODE, RATE_GENERATOR);
	outpb(PORT_PIT_TIMER0, (byte)(TIMER_FREQ / kHZ));
	outpb(PORT_PIT_TIMER0, (byte)((TIMER_FREQ / kHZ) >> 8));
	// Chan 2: for PC Speaker
	outpb(PORT_TIMER_MODE, 0b10110110);
	outpb(PORT_PIT_TIMER2, (byte)(TIMER_FREQ / BEEP_COUNTER));
	outpb(PORT_PIT_TIMER2, (byte)((TIMER_FREQ / BEEP_COUNTER) >> 8));
	//
	i8259Master_Enable(DEV_MAS_PIT);
}

#endif
