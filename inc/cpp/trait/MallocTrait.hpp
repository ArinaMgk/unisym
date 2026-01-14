// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Trait) Memory Allocator
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

// Application
// - mempool

#ifndef _INCPP_TRAIT_Malloc
#define _INCPP_TRAIT_Malloc
#include "../unisym"
#include "../string"
namespace uni {
	namespace trait {
		class Malloc {
		public:
			byte alignment;// exponent of two. 0 for no alignment.
		public:
			~Malloc() noexcept = default;
			//
			virtual void* allocate(stduint size) = 0;
			virtual bool deallocate(void* ptr, stduint size = 0 _Comment(zero_for_block)) = 0;
			virtual void* reallocate(void* ptr, stduint old_size, stduint size) {
				// general method
				if (void* newptr = allocate(size)) {
					MemCopyN(newptr, ptr, old_size);
					deallocate(ptr);
				}
				else return nullptr;
			}
		};
	}
}


#endif // _INCPP_TRAIT_Malloc
