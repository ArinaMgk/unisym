// ASCII CPL TAB4 CRLF
// Docutitle: [Device] FDCAN
// Developer: @dosconio
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

#include "../../../../inc/c/driver/FDCAN.h"

using namespace uni;

#if defined(_MCU_STM32H7x)
_ESYM_C{
	void FDCAN1_IT0_IRQHandler(void) { FDCAN1.irqHandler(); }
	void FDCAN1_IT1_IRQHandler(void) { FDCAN1.irqHandler(); }
	void FDCAN2_IT0_IRQHandler(void) { FDCAN2.irqHandler(); }
	void FDCAN2_IT1_IRQHandler(void) { FDCAN2.irqHandler(); }
	// Clock calibration unit is a single global unit shared by FDCAN1/2:
	// dispatch to both instances so either registered callback can run.
	void FDCAN_CAL_IRQHandler(void) { FDCAN1.irqHandler(); FDCAN2.irqHandler(); }
}
#endif
