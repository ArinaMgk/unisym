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

#if defined(_MCU_STM32F1x) || defined(_MCU_STM32F4x)
namespace uni {
	// BELONG cortex_m3 and cortex_m4
	static uint32_t NVIC_EncodePriority(uint32_t PriorityGroup, uint32_t PreemptPriority, uint32_t SubPriority)
	{
		PriorityGroup &= 0x7;
		uint32_t PreemptPriorityBits = ((7UL - PriorityGroup) > _NVIC_PRIO_BITS) ? _NVIC_PRIO_BITS : (uint32_t)(7UL - PriorityGroup);
		uint32_t SubPriorityBits = ((PriorityGroup + _NVIC_PRIO_BITS) < (uint32_t)7UL) ? (uint32_t)0UL : (uint32_t)((PriorityGroup - 7UL) + _NVIC_PRIO_BITS);
		return ((PreemptPriority & (uint32_t)((1UL << (PreemptPriorityBits)) - 1UL)) << SubPriorityBits) |
			((SubPriority & (uint32_t)((1UL << (SubPriorityBits)) - 1UL)));
	}
}
#endif


#if defined(_MCU_STM32F1x)
namespace uni {

	NVIC_t NVIC;

	_TODO uint32 NVIC_t::getPriorityGroup() {
		return 0; //{AI???}(uint32)((map->CTRL & (uint32)0x07000000) >> (uint32)28);
	}

	_TODO uint32 NVIC_t::setPriorityGroup(...) { return 0; }

	void NVIC_t::setPriority(Request_t req, uint32 priority) {
		const uint32 IRQ = (uint32)(uint8)req;
		uint8 writ = (priority << (8U - _NVIC_PRIO_BITS)) & (uint32)0xFF;
		if ((sint32)req >= 0)
			map->IP[IRQ] = writ;
		else
			scbmap->SHP[(IRQ & (uint32)0xF) - (uint32)4] = writ;
	}

	void NVIC_t::setPriority(Request_t req, uint32 prepriority, uint32 subpriority) {
		setPriority(req, NVIC_EncodePriority(getPriorityGroup(), prepriority, subpriority));
	}

	void NVIC_t::setAble(Request_t req, bool ena) {
		const stduint req_no = _IMM(req);
		if (!req_no) return;
		if (ena) {
			this->map->ISER[req_no >> 5UL] |= ((uint32_t)1 << (req_no & 0x1FUL));
		}
		else {
			this->map->ICER[req_no >> 5UL] |= ((uint32_t)1 << (req_no & 0x1FUL));
			//{MORE TODO}
		}
	}
}

#elif defined(_MCU_STM32F4x)

namespace uni {

	NVIC_t NVIC;

	_TODO uint32 NVIC_t::getPriorityGroup() {
		return 0; //{AI???}(uint32)((map->CTRL & (uint32)0x07000000) >> (uint32)28);
	}

	void NVIC_t::setPriority(Request_t req, uint32 priority) {
		const uint32 IRQ = (uint32)(uint8)req;
		uint8 writ = (priority << (8U - _NVIC_PRIO_BITS)) & (uint32)0xFF;
		if ((sint32)req >= 0)
			*getTable(IRQ) = writ;
		else
			scbmap->SHP[(IRQ & (uint32)0xF) - (uint32)4] = writ;
	}

	void NVIC_t::setPriority(Request_t req, uint32 prepriority, uint32 subpriority) {
		setPriority(req, NVIC_EncodePriority(getPriorityGroup(), prepriority, subpriority));
	}

}

#elif defined(_MCU_STM32H7x)
#define _CortexM7_SCB_TEMP
#include "../../../inc/c/prochip/CortexM7.h"

namespace uni {

	NVIC_t NVIC;

	Reference NVIC_t::operator[](NVICReg::NVICReg idx) {
		return _NVIC_BASE + _IMMx4(idx);
	}

	void NVIC_t::setPriorityGroup(stduint bitsof_prepriority) {
		uint32_t reg_value;
		uint32_t PriorityGroupTmp = ((7 - bitsof_prepriority) & 0x07UL);     /* only values 0..7 are used          */
		reg_value = SCB->AIRCR;                                              /* read old register configuration    */
		reg_value &= ~_IMM(SCB_AIRCR_VECTKEY_Msk | SCB_AIRCR_PRIGROUP_Msk);  /* clear bits to change               */
		reg_value = (reg_value |
			((uint32_t)0x5FAUL << SCB_AIRCR_VECTKEY_Pos) |
			(PriorityGroupTmp << 8U)); // Insert write key and priorty group
		SCB->AIRCR = reg_value;
	}

	void NVIC_t::setPriority(Request_t req, uint32 priority) {
		const uint32 IRQ = (uint32)(uint8)req;
		uint8 writ = (priority << (8U - _NVIC_PRIO_BITS)) & (uint32)0xFF;
		if ((sint32)req >= 0)
			*getTable(IRQ) = writ;
		else
			SCB->SHPR[(IRQ & (uint32)0xF) - (uint32)4] = writ;
	}

}





#endif
