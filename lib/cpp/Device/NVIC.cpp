// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Nested Vectored Interrupt Controller
// Codifiers: @dosconio: 20240511
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

#include "../../../inc/cpp/Device/NVIC"

namespace uni {
	void NVIC_t::setPriority(Request_t req, uint32 priority) {
		const uint32 IRQ = (uint32)(uint8)req;
		uint8 writ = (priority << (8U - _NVIC_PRIO_BITS)) & (uint32)0xFF;
		if ((sint32)req >= 0)
			map->IP[IRQ] = writ;
		else
			scbmap->SHP[(IRQ & (uint32)0xF) - (uint32)4] = writ;
	}
}
