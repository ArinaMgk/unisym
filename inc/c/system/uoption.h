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


#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)|| defined(_MPU_STM32MP13)
	#define _MCU_STM32
	#ifndef __BITS__
	#define __BITS__ 32
	#endif
	#define Architecture_Value Architecture_ARM
#elif defined(_MCU_CW32F030) || defined(_MCU_CW32F003)
	#define _MCU_CW32
	#ifndef __BITS__
	#define __BITS__ 32
	#endif
#elif defined(_MCU_MSP432P4)
	#define _MCU_MSP432
	#ifndef __BITS__
	#define __BITS__ 32
	#endif
	//
	#define ROM_APITABLE ((uint32*)0x02000800)
#endif

#if defined(Architecture_Value)
#if defined(_MPU_STM32MP13)
#define Architecture_ARMv_Value Archit_ARM_v7
#endif
#endif

// _SUPPORT_GPIO
#ifdef _MCU_STM32
#define _SUPPORT_GPIO
#endif
#if defined(_MCU_CW32)
#define _SUPPORT_GPIO
#endif
#if defined(_MCU_MSP432)
#define _SUPPORT_GPIO
#endif

#endif
