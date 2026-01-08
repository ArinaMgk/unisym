// ASCII CPL TAB4 CRLF
// Docutitle: (Module) Real Time Clock
// Codifiers: @dosconio: 20240220 ~ 20240429
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

#ifndef _INC_DEVICE_RealtimeClock
#define _INC_DEVICE_RealtimeClock

#include "../stdinc.h"
#include "../datime.h"

#if defined(_MCCA) && (_MCCA==0x8616||_MCCA==0x8632)

#ifdef _INC_CPP
extern "C" {
#endif

void RTC_Init();

void CMOS_Readtime(struct tm* time);

#ifdef _INC_CPP
}
#endif


#elif defined(_MCU_STM32F1x) // C++





#endif

#endif
