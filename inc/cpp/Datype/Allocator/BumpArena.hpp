// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: Bump Arena Allocator
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Host[Allocation]
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

#ifndef _INCPP_DATYPE_ALLOCATOR_BUMPARENA
#define _INCPP_DATYPE_ALLOCATOR_BUMPARENA

#include "../../trait/MallocTrait.hpp"

namespace uni {

	class BumpArena : public trait::Malloc {
	public:
		explicit BumpArena(stduint block_size = 0x1000);
		~BumpArena();

		BumpArena(const BumpArena&) = delete;
		BumpArena& operator=(const BumpArena&) = delete;

		BumpArena(BumpArena&& other) noexcept;
		BumpArena& operator=(BumpArena&& other) noexcept;

		void* allocate(stduint size, stduint alignment = 0,
			stduint boundary = 0) override;
		bool deallocate(void* ptr, stduint size = 0) override;
		void* reallocate(void* ptr, stduint old_size, stduint size,
			stduint alignment = 0) override;

		void* AllocateZero(stduint size, stduint alignment = 0,
			stduint boundary = 0);
		char* CopyString(const char* string, stduint length = 0);

		void Reset();
		void Clear();

		stduint Used() const { return used_size; }
		stduint Capacity() const { return capacity_size; }
		stduint BlockCount() const { return block_count; }

	private:
		struct Block;

		Block* root_block;
		Block* current_block;
		Block* tail_block;
		stduint default_block_size;
		stduint used_size;
		stduint capacity_size;
		stduint block_count;

		static bool AddSize(stduint left, stduint right, stduint& result);
		static bool DecodeScale(stduint exponent, stduint& scale);
		static bool RequiredCapacity(stduint size, stduint alignment,
			stduint boundary, stduint& capacity);
		static void* AllocateFrom(Block* block, stduint size,
			stduint alignment, stduint boundary, stduint& consumed);
		bool AppendBlock(stduint minimum_capacity);
	};

}

#endif
