// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) MMU
// Codifiers: @dosconio: 20241209 ~ <Last-check> 
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

#include "../../../inc/cpp/Device/MMU"

#ifdef _MPU_STM32MP13
#include "../../../inc/c/proctrl/ARM/cortex_a7.h"

#define statin inline static

// TLB Invalidate All
statin void __set_TLBIALL(uint32 value)
{
	__set_CP(15, 0, value, 8, 7, 0);
}

namespace uni {
	MMU_t MMU;

	void MMU_t::InvalidateTLB() const {
		__set_TLBIALL(0);
		__DSB();// ensure completion of the invalidation
		__ISB();// ensure instruction fetch path sees new state
	}
	
}

#endif
