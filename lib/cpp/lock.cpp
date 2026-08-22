// ASCII C++2x TAB4 CRLF
// AllAuthor: @ArinaMgk
// ModuTitle: Basic Modules
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

#include "../../inc/cpp/lock"

#if defined(_MCU_STM32H7x)
#include "../../inc/cpp/Device/Lock/HSEM.hpp"

namespace uni {
	// H7: one HSEM with 32 semaphores; ids >= 32 are out of range.
	#define _HARDLOCK_HSEM_SEM_COUNT 32

	// Acquire the lock via HSEM1. process_id = -1 selects fast 1-step,
	// otherwise 2-step with that process id.
	bool Hardlock::Acquire(stdsint process_id) {
		if (sem_id >= _HARDLOCK_HSEM_SEM_COUNT) return false;
		return HSEM1.Take(sem_id, process_id);
	}

	// Non-blocking fast acquire (1-step).
	bool Hardlock::TryAcquire() {
		if (sem_id >= _HARDLOCK_HSEM_SEM_COUNT) return false;
		return HSEM1.Take(sem_id, -1);
	}

	// Release the lock.
	void Hardlock::Release(stduint process_id) {
		if (sem_id >= _HARDLOCK_HSEM_SEM_COUNT) return;
		HSEM1.Release(sem_id, process_id);
	}

	// Check whether the lock is held.
	bool Hardlock::isTaken() {
		if (sem_id >= _HARDLOCK_HSEM_SEM_COUNT) return false;
		return HSEM1.isTaken(sem_id);
	}
}
#endif

