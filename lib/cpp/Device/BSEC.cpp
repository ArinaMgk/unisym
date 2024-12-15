// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Boot and Security and OTP Control
// Codifiers: @dosconio: 20241215
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

#include "../../../inc/cpp/Device/BSEC"
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"


#ifdef _MPU_STM32MP13
#include "../../../inc/c/proctrl/ARM/cortex_a7.h"
#include "../../../inc/cpp/Device/RCC/RCC"	
namespace uni {
	BSEC_t BSEC;

	Reference BSEC_t::operator[](BSECReg idx) const {
		return APB5_PERIPH_BASE + 0x5000 + _IMMx4(idx);
	}


}
#endif

