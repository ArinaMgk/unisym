// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Analog-Digital Converter
// Codifiers: @dosconio: 20240709~;
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: GPIO and other ADC Channnels
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
// inner include guard, do not inc this to feel hard

#if defined(_INC_Device_ADC) && !defined(_INC_Device_ADC_ONCE)
#define _INC_Device_ADC_ONCE
#include "../../stdinc.h"
// CR2
#define _ADC_CR2_POS_ADON   0
#define _ADC_CR2_POS_CONT   1
#define _ADC_CR2_POS_DMA    8
#define _ADC_CR2_POS_ALIGN  11

#define _ADC_SQR1_POS_L     20 // len 4b

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)

#endif

#if defined(_MCU_STM32F4x)
// C++ only
namespace uni {
	namespace ADCReg {
		struct BLK_CR1 {
			// 0x000000XX
			stduint AWDCH : 5, EOCIE : 1, AWDIE : 1, JEOCIE : 1;
			// 0x0000XX00
			stduint SCAN : 1, AWDSGL : 1, JAUTO : 1, DISCEN : 1, JDISCEN : 1, DISCNUM : 3;
			// 0x00XX0000
			stduint _RESERVED_CR_1 : 6, JAWDEN : 1, AWDEN : 1;
			// 0xXX000000
			stduint RES : 2, OVERIE : 1, _RESERVED_CR_2 : 5;
		};
	}
}

#endif

#endif // _INC_Device_ADC
