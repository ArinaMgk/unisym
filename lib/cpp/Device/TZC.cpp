// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) TZC TrustZone Address
// Codifiers: @dosconio: 20241213 ~ <Last-check> 
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

#include "../../../inc/cpp/Device/TZC"
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"

#ifdef _MPU_STM32MP13
#include "../../../inc/c/proctrl/ARM/cortex_a7.h"
#include "../../../inc/cpp/Device/RCC/RCC"	
namespace uni {
	TZC_t TZC;
	ETZPC_t ETZPC;

	Reference TZC_t::operator[](TZCReg idx) const {
		return APB5_PERIPH_BASE + 0x6000 + _IMMx4(idx);
	}

	void TZC_t::enClock(bool ena) const {
		RCC[ena ? RCCReg::MP_APB5ENSETR : RCCReg::MP_APB5ENCLRR] = _IMM1S(11);// TZCEN
	}

	//

	void ETZPC_t::enClock(bool ena) const {
		RCC[ena ? RCCReg::MP_APB5ENSETR : RCCReg::MP_APB5ENCLRR] = _IMM1S(13);// ETZPCEN
	}



}
#endif

