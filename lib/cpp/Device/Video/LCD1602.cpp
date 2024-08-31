// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) LCD1602 
// Codifiers: @dosconio: RFB3 ~ <Last-check> 
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

#include "../../../../inc/c/driver/LCD1602.h"

#if defined(_INC_CPP) && defined(_SUPPORT_GPIO)

static void inline_LCD_delay(void) {
	volatile int i = 0x100;
	while(i--);
}

namespace uni {

	LCD1602_IIC_t::LCD1602_IIC_t(GPIO_Pin& SDA, GPIO_Pin& SCL, void (*delay_ms)(stduint ms)) :
		IIC(SDA, SCL), delay_ms(delay_ms) 
	{
		IIC.func_delay = inline_LCD_delay;
	}


	
}

#endif
