// ASCII CPP TAB4 CRLF
// Docutitle: (Driver) SRAM
// Codifiers: @dosconio: 20240723
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

#include "../../../inc/c/driver/SRAM.h"

#ifdef _MPU_STM32MP13
#include "../../../inc/c/proctrl/ARM/cortex_a7.h"
#include "../../../inc/cpp/Device/RCC/RCC"
#include "../../../inc/cpp/MCU/_ADDRESS/ADDR-STM32.h"
namespace uni {
	BKPSRAM_t BKPSRAM;

	#define impl(x) x BKPSRAM_t
	

	impl(stduint)::getAddress() const {
		return AHB5_PERIPH_BASE;
	}
	
	impl(void)::enClock(bool ena) const {
		RCC[ena ? RCCReg::MP_AHB5ENSETR : RCCReg::MP_AHB5ENCLRR] = _IMM1S(8);// BKPSRAMEN
	}




}
#endif
