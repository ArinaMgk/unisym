// ASCII CPP TAB4 CRLF
// Docutitle: Interrupt List Abstract
// Codifiers: @dosconio: 20240528
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
#include "../../../../inc/cpp/interrupt"
#include "../../../../inc/cpp/Device/EXTI"
extern "C" {

	#if 0
	//
	#elif defined(_MCU_STM32F10x)
	Handler_t FUNC_EXTI[16] = { 0 };

	static void _HandlerIRQ_EXTIx(byte x) {
		if (uni::EXTI::Pending & (1 << x)) {
			// GPIO_PIN_x
			if (FUNC_EXTI[x]) FUNC_EXTI[x]();
			uni::EXTI::Pending = (1 << x);
		}
	}
	void EXTI0_IRQHandler(void) {_HandlerIRQ_EXTIx(0);}
	void EXTI1_IRQHandler(void) {_HandlerIRQ_EXTIx(1);}
	void EXTI2_IRQHandler(void) {_HandlerIRQ_EXTIx(2);}
	void EXTI3_IRQHandler(void) {_HandlerIRQ_EXTIx(3);}
	void EXTI4_IRQHandler(void) {_HandlerIRQ_EXTIx(4);}
	void EXTI9_5_IRQHandler(void) {
		for (byte i = 5; i < 9; i++) _HandlerIRQ_EXTIx(i);
	}
	void EXTI15_10_IRQHandler(void) {
		for (byte i = 10; i < 16; i++) _HandlerIRQ_EXTIx(i);
	}



	#else

	#endif
}
