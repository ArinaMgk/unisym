// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: PLL Module and RCC PLL Controller
// Codifiers: @dosconio: 20241116
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

#include "../../../../inc/cpp/Device/RCC/RCC"
#include "../../../../inc/cpp/Device/Flash"
#include "../../../../inc/cpp/Device/SysTick"
#include "../../../../inc/c/driver/RCC/RCC-registers.hpp"

extern "C" stduint HSE_VALUE;
extern "C" stduint HSI_VALUE;

namespace uni {
	using namespace RCCReg;
#include "../../../../inc/c/driver/RCC/PLL.hpp"
}
