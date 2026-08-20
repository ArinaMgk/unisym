// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.Lock) Hardware Semaphore, HSEM
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

#include "../../../../inc/cpp/Device/Lock/HSEM.hpp"
#include "../../../../inc/cpp/Device/NVIC"
#include "../../../../inc/cpp/Device/Interrupt/interrupt_tab.h"
#include "../Interrupt/interrupt_hsem.hpp"

namespace uni {
#if defined(_MCU_STM32H7x)

	HSEM HSEM1;

	// RCC AHB4ENR.HSEMEN (bit 25)
	void HSEM::enClock(bool ena) {
		Reference(_RCC_AHB4ENR_ADDR).setof(25, ena);
	}

	// Enable clock and reset HSEM configuration (interrupt enable/clear/key)
	bool HSEM::setMode() {
		enClock();
		self[HSEMReg::IER] = 0;
		self[HSEMReg::ICR] = 0xFFFFFFFFU;
		self[HSEMReg::KEYR] = 0;
		return true;
	}

	// AKA HAL_HSEM_Take / HAL_HSEM_FastTake (process_id == -1 selects fast path)
	bool HSEM::Take(stduint sem_id, stdsint process_id) {
		if (sem_id > _HSEM_SEMID_MAX) return false;
		if (process_id == -1) {
			// fast 1-step: read RLR, success if LOCK set and MasterID matches
			return RLR(sem_id) == ((stduint(master_id) << _HSEM_RLR_POS_MASTERID) | _IMM1S(_HSEM_RLR_POS_LOCK));
		}
		// 2-step: write R with PROCID|MASTERID|LOCK, then readback verify
		if (stduint(process_id) > _HSEM_PROCESSID_MAX) return false;
		stduint expect = (stduint(process_id) & 0xFF)
			| (stduint(master_id) << _HSEM_R_POS_MASTERID)
			| _IMM1S(_HSEM_R_POS_LOCK);
		R(sem_id) = expect;
		return R(sem_id) == expect;
	}

	// AKA HAL_HSEM_IsSemTaken
	bool HSEM::isTaken(stduint sem_id) {
		if (sem_id > _HSEM_SEMID_MAX) return false;
		return R(sem_id).bitof(_HSEM_R_POS_LOCK);
	}

	// AKA HAL_HSEM_Release
	void HSEM::Release(stduint sem_id, stduint process_id) {
		if (sem_id > _HSEM_SEMID_MAX) return;
		if (process_id > _HSEM_PROCESSID_MAX) return;
		// write R without LOCK bit
		R(sem_id) = (process_id & 0xFF) | (stduint(master_id) << _HSEM_R_POS_MASTERID);
	}

	// AKA HAL_HSEM_ReleaseAll (uses master_id field; key guards the release)
	void HSEM::ReleaseAll(stduint key) {
		if (key > _HSEM_CLEAR_KEY_MAX) return;
		if (master_id > 0xFF) return;
		self[HSEMReg::CR] = ((key << _HSEM_CR_POS_KEY) & 0xFFFF0000U)
			| (stduint(master_id) << _HSEM_CR_POS_MASTERID);
	}

	// AKA HAL_HSEM_SetClearKey
	void HSEM::setClearKey(stduint key) {
		self[HSEMReg::KEYR].maset(_HSEM_KEYR_POS_KEY, 16, key);
	}

	// AKA HAL_HSEM_GetClearKey
	stduint HSEM::getClearKey() {
		return self[HSEMReg::KEYR].masof(_HSEM_KEYR_POS_KEY, 16);
	}

	// AKA HAL_HSEM_ActivateNotification
	void HSEM::ActivateNotification(stduint sem_mask) {
		self[HSEMReg::IER] |= sem_mask;
	}

	// AKA HAL_HSEM_DeactivateNotification
	void HSEM::DeactivateNotification(stduint sem_mask) {
		self[HSEMReg::IER] &= ~sem_mask;
	}

	// ---- RuptTrait (NVIC + IRQ_HSEM1) ----
	void HSEM::setInterrupt(Handler_t f) const {
		FUNC_HSEM[0] = f;
	}
	void HSEM::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(IRQ_HSEM1, preempt, sub_priority);
	}
	void HSEM::enInterrupt(bool enable) const {
		NVIC.setAble(IRQ_HSEM1, enable);
	}

#endif // _MCU_STM32H7x
}
