// ASCII C/C++ TAB4 CRLF
// Docutitle: (Processor, not MCU and not ARCH) Cortex-M3
// Codifiers: @dosconio: 20240529
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

#ifndef _INC_Processor_CortexM3
#define _INC_Processor_CortexM3

//{TODO} core_cm3.h
//{TODO} up-abstract from STM32F1

#include "../stdinc.h"

#include "_COM_CORTEX_M_3_4.h"


#define _ITM_ADDR            (0xE0000000UL)
#define _DWT_ADDR            (0xE0001000UL)
#define _TPI_ADDR            (0xE0040000UL)
#define _CoreDebug_ADDR      (0xE000EDF0UL)




#endif
