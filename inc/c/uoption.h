// ASCII C/C++ TAB4 CRLF
// Docutitle: Macro Options
// Codifiers: @dosconio: 20240423 ~ 20240423
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Descripts: This file is supplementary for C/C++, while Magice builds these in.
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

#ifndef _INC_Options
#define _INC_Options

#ifndef _MCU_STM32
#if defined(_MCU_STM32F103VE) || defined(_MCU_STM32F1x)
	#define _MCU_STM32
#elif defined(_MCU_STM32F407ZE) || defined(_MCU_STM32F4x)
	#define _MCU_STM32
#endif
#endif

// _SUPPORT_GPIO
#ifdef _MCU_STM32
#define _SUPPORT_GPIO
#endif
#if defined(_MCU_CW32F030) || defined(_MCU_CW32F003)
#define _SUPPORT_GPIO
#endif

#endif
