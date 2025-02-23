// ASCII C/C++ TAB4 CRLF
// Docutitle: (Device) Flash
// Codifiers: @dosconio: ~ 20240718
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


#include "../../../inc/cpp/Device/Flash"
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"

namespace uni {
#if 0
#elif defined(_MCU_STM32F4x)
	Flash_t Flash;
#elif defined(_MCU_STM32H7x)
	Flash_t Flash;

	Reference Flash_t::operator[](FlashReg::FlashReg idx) {
		return _Flash_ADDR + _IMMx4(idx);
	}

#endif
}
