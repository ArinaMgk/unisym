// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Internal Voltage Reference Buffer, VREFBUF
// Codifiers: @ArinaMgk
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

#include "../../../inc/cpp/Device/VREFBUF"

namespace uni {
#if defined(_MCU_STM32H7x)

	#define _VREFBUF_TIMEOUT_VALUE 10U // ms

	VREFBUF VREF;

	// AKA HAL_SYSCFG_VREFBUF_VoltageScalingConfig
	void VREFBUF::setVoltageScaling(VREFBUFScale scale) {
		self[VREFBUFReg::CSR].maset(_VREFBUF_CSR_POS_VRS, 3, (stduint)scale);
	}

	// AKA HAL_SYSCFG_VREFBUF_HighImpedanceConfig
	void VREFBUF::setHighImpedance(bool ena) {
		self[VREFBUFReg::CSR].setof(_VREFBUF_CSR_POS_HIZ, ena);
	}

	// AKA HAL_SYSCFG_VREFBUF_TrimmingConfig
	void VREFBUF::setTrimming(stduint value) {
		self[VREFBUFReg::CCR].maset(_VREFBUF_CCR_POS_TRIM, 6, value);
	}

	// AKA HAL_SYSCFG_EnableVREFBUF
	bool VREFBUF::enAble(bool ena) {
		if (ena) {
			self[VREFBUFReg::CSR].setof(_VREFBUF_CSR_POS_ENVR);
			uint64 tickstart = SysTick::getTick();
			while (!self[VREFBUFReg::CSR].bitof(_VREFBUF_CSR_POS_VRR)) {
				if ((SysTick::getTick() - tickstart) > _VREFBUF_TIMEOUT_VALUE) return false;
			}
			return true;
		}
		self[VREFBUFReg::CSR].rstof(_VREFBUF_CSR_POS_ENVR);
		return true;
	}

	// AKA HAL_SYSCFG_DisableVREFBUF
	void VREFBUF::enDisable() {
		self[VREFBUFReg::CSR].rstof(_VREFBUF_CSR_POS_ENVR);
	}

#endif // _MCU_STM32H7x
}
