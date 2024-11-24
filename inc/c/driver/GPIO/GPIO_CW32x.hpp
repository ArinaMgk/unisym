// ASCII CPP TAB4 CRLF
// Docutitle: (Device) General Purpose Input Output, for CW32x
// Codifiers: @dosconio: 20240716
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
//! only included by inner UNISYM `.../GPIO`
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

#ifndef _INC_INN_GPIO_CW32
#define _INC_INN_GPIO_CW32
#if defined(_MCU_CW32F030)

#define defa_speed GPIOSpeed::Low

enum class GPIOReg {
	DIR = 0, OPENDRAIN, SPEED, PDR,
	PUR, AFRH, AFRL, ANALOG,
	DRIVER, RISEIE, FALLIE, HIGHIE,
	LOWIE, ISR, ICR, LCKR,
	FILTER, REV0, REV1, REV2,
	IDR, ODR, BRR, BSRR,
	TOG
};

#endif
#endif
