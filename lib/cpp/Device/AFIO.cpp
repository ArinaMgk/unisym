// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Alternate Function I/O
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

#include "../../../inc/cpp/Device/AFIO"
namespace uni {
#if 0

#elif defined(_MCU_STM32F1x)

	Reference AFIO::EventCtrlReg(_AFIO_ADDR + 0x00);// AFIO_EVCR
	Reference AFIO::MapReg(_AFIO_ADDR + 0x04);// AFIO_MAPR
	Reference AFIO::ExternInterruptCfgs[4] = {// AFIO_EXTICRx
		Reference(_AFIO_ADDR + 0x08),Reference(_AFIO_ADDR + 0x0C),
		Reference(_AFIO_ADDR + 0x10),Reference(_AFIO_ADDR + 0x14)
	};

#elif defined(_MCU_STM32F4x)

	Reference SYSCFG::MEMRM(_SYSCFG_ADDR);
	Reference SYSCFG::PMC(_SYSCFG_ADDR + 0x04);
	Reference SYSCFG::ExternInterruptCfgs[4] = {
		Reference(_SYSCFG_ADDR + 0x08),
		Reference(_SYSCFG_ADDR + 0x0C),
		Reference(_SYSCFG_ADDR + 0x10),
		Reference(_SYSCFG_ADDR + 0x14)
	};
	Reference SYSCFG::CMPCR(_SYSCFG_ADDR + 0x20);

#endif
}
