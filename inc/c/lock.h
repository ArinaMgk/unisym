// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) Lock
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

#ifndef _INC_LOCKSWAP
#define _INC_LOCKSWAP

#include "./stdinc.h"

//    mutex: os-layer, sleep if the lock is locked by another thread
// spinlock: usr-layer, busy-wait or give-up if the lock is locked by another thread

byte LockSwap8(byte* b, byte val);


//{unchk} Below: we use 0 for unlocked and not 0 for locked

inline static byte LockMutexNeed(byte* loc) {
	return LockSwap8(loc, 1);
}
inline static void LockMutexFree(byte* loc) {
	*loc = 0;
}

inline static void LockSpinNeed(byte* loc) {
	while (LockSwap8(loc, 1));
}
inline static void LockSpinFree(byte* loc) {
	*loc = 0;
}


#endif
