// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Trait) Stroage
// Codifiers: @dosconio: 20250104
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

// a Block default { 512 bytes }

// - Harddisk
// - SDCard

#ifndef _INCPP_TRAIT_Storage
#define _INCPP_TRAIT_Storage
#include "../unisym"
#include "../trait/BlockTrait.hpp"

// x86
struct PartitionTableX86 {
	byte status;
	byte head_start;
	word sector_start : 6;
	word cylinder_start : 10;
	byte type;// system id
	byte head_end;
	word sector_end : 6;
	word cylinder_end : 10;
	uint32 lba_start;
	uint32 lba_count;
};

namespace uni {
	class StorageTrait : public BlockTrait {
	public:
		void* Block_buffer;// for byte access
		
		// byte read. -1 for error
		virtual int operator[](uint64 bytid) = 0;

	};

}


#endif
