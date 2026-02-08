// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) Mempool
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

#include "../../../inc/c/mempool.h"

namespace uni {

	void* Mempool::allocate(stduint size, stduint alignment) {
		// SinglePool* crtpool = &pool_available;
	}
	bool Mempool::deallocate(void* ptr, stduint size _Comment(zero_for_block)) {
		_TODO
	}



	
}
