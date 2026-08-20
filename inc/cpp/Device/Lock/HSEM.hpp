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

#ifndef _INC_DEVICE_LOCK_HSEM
#define _INC_DEVICE_LOCK_HSEM

#include "../../../c/stdinc.h"
#include "../../reference"
#include "../../interrupt"
#include "../RCC/RCCAddress"

#if defined(_MCU_STM32H7x)

// global ISR entry (extern "C"; defined in interrupt_hsem.hpp), friended below
extern "C" void HSEM1_IRQHandler();

namespace uni {

	// AKA HSEM_TypeDef register map (byte offsets from 0x58026400)
	enum class HSEMReg : unsigned {
		R0 = 0x00,   // 2-step lock registers R[0..31]
		RLR0 = 0x80, // 1-step lock registers RLR[0..31]
		IER = 0x100, // interrupt enable
		ICR = 0x104, // interrupt clear
		ISR = 0x108, // interrupt status
		MISR = 0x10C,// interrupt masked status
		// 0x110-0x13C reserved
		CR = 0x140,  // semaphore clear (release all)
		KEYR = 0x144,// semaphore clear key
	};

	// HSEM_R / HSEM_RLR bit positions
	#define _HSEM_R_POS_PROCID   0  // 8b process id
	#define _HSEM_R_POS_MASTERID 8  // 8b master id
	#define _HSEM_R_POS_LOCK     31 // 1b lock
	#define _HSEM_RLR_POS_MASTERID 8
	#define _HSEM_RLR_POS_LOCK     31

	// HSEM_CR bit positions
	#define _HSEM_CR_POS_MASTERID 8  // 8b
	#define _HSEM_CR_POS_KEY      16 // 16b

	// HSEM_KEYR bit positions
	#define _HSEM_KEYR_POS_KEY 16 // 16b

	// limits (AKA IS_HSEM_*)
	#define _HSEM_SEMID_MAX     31
	#define _HSEM_PROCESSID_MAX 255
	#define _HSEM_CLEAR_KEY_MAX 0xFFFF

	// Hardware semaphore (H7 only; AKA HSEM_HandleTypeDef).
	// 32 hardware semaphores for inter-master mutual exclusion (CM7/CM4/DMA...),
	// no interrupt disable needed (hardware atomic read-modify-write).
	class HSEM {
	public:
		// MasterID of the current core, used by Take/Release/ReleaseAll.
		// H7 HAL uses CM7 = 3; set to 4 for CM4 on dual-core parts.
		byte master_id = 3;

		// released-semaphore notification callback (AKA HAL_HSEM_FreeCallback);
		// receives the mask of released semaphores
		_tocall_ft FreeHandler = 0;

		// Enable clock and reset HSEM configuration
		bool setMode();

		// RCC AHB4ENR.HSEMEN (bit 25)
		void enClock(bool ena = true);
		// Take a semaphore. process_id = -1 selects the fast 1-step method (RLR);
		// otherwise the 2-step method (R write + readback verify) with that process id.
		bool Take(stduint sem_id, stdsint process_id);
		// Check whether a semaphore is taken (LOCK bit only, no acquire).
		bool isTaken(stduint sem_id);
		// Release a semaphore (write R without LOCK bit).
		void Release(stduint sem_id, stduint process_id);
		// Release all semaphores held by master_id, guarded by key.
		void ReleaseAll(stduint key);
		// Set / get the clear key (KEYR).
		void setClearKey(stduint key);
		stduint getClearKey();
		// Enable / disable release notification for a semaphore mask (IER).
		void ActivateNotification(stduint sem_mask);
		void DeactivateNotification(stduint sem_mask);

		// register access: R[n] at 0x00+n*4, RLR[n] at 0x80+n*4
		Reference operator[](HSEMReg reg) const { return Reference(0x58026400 + _IMM(reg)); }
		Reference R(stduint n) const { return Reference(0x58026400 + n * 4); }
		Reference RLR(stduint n) const { return Reference(0x58026400 + 0x80 + n * 4); }

		// RuptTrait (NVIC + IRQ_HSEM1)
		_COM_DEF_Interrupt_Interface();

	protected:
		// ISR entry (global, extern "C", defined in interrupt_hsem.hpp)
		friend void ::HSEM1_IRQHandler();
	};

	extern HSEM HSEM1;

}

#endif // _MCU_STM32H7x

#endif // _INC_DEVICE_LOCK_HSEM
