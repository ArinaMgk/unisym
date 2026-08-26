// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Debug MCU, DBGMCU
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

#include "../../../inc/cpp/Device/DBG"

namespace uni {
#if defined(_MCU_STM32H7x)

	DBG DBGMCU;

	// AKA HAL_Enable/DisableDBGSleepMode
	void DBG::enDBGSleep(bool ena) {
		self[DBGReg::CR].setof(_DBG_CR_POS_SLEEPD1, ena);
	}

	// AKA HAL_Enable/DisableDBGStopMode
	void DBG::enDBGStop(bool ena) {
		self[DBGReg::CR].setof(_DBG_CR_POS_STOPD1, ena);
	}

	// AKA HAL_Enable/DisableDBGStandbyMode
	void DBG::enDBGStandby(bool ena) {
		self[DBGReg::CR].setof(_DBG_CR_POS_STANDBYD1, ena);
	}

	// AKA HAL_Enable/DisableDomain3DBGStopMode
	void DBG::enDBGStopDomain3(bool ena) {
		self[DBGReg::CR].setof(_DBG_CR_POS_STOPD3, ena);
	}

	// AKA HAL_Enable/DisableDomain3DBGStandbyMode
	void DBG::enDBGStandbyDomain3(bool ena) {
		self[DBGReg::CR].setof(_DBG_CR_POS_STANDBYD3, ena);
	}

#endif // _MCU_STM32H7x
}
