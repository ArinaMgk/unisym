// ASCII CPP TAB4 CRLF
// Docutitle: Interrupt
// Codifiers: @dosconio: 20240527
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

#include "../../inc/cpp/interrupt"

namespace uni {
// Occupiers

	void RuptTrait::setInterrupt(Handler_t fn) { (void)fn; }
	void RuptTrait::setInterruptPriority(byte preempt, byte sub_priority) { (void)preempt; (void)sub_priority; }
	void RuptTrait::enInterrupt(bool enable) { (void)enable; }


#if 0
//
#elif defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)

Reference EXTI::MaskInterrupt(_EXTI_ADDR + 0x00);// aka EXTI_IMR, default 0x00000000

Reference EXTI::MaskEvent(_EXTI_ADDR + 0x04);// aka EXTI_EMR, default 0x00000000

Reference EXTI::TriggerRising(_EXTI_ADDR + 0x08);// aka EXTI_RTSR, default 0x00000000

Reference EXTI::TriggerFalling(_EXTI_ADDR + 0x0C);// aka EXTI_FTSR, default 0x00000000

Reference EXTI::Softrupt(_EXTI_ADDR + 0x10);// aka EXTI_SWIER, default 0x00000000

Reference EXTI::Pending(_EXTI_ADDR + 0x14);// aka EXTI_PR, default undefined

	
#else
// may for osdev?
#endif
}
