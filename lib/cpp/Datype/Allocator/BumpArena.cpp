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

#include "../../../../inc/cpp/Datype/Allocator/BumpArena.hpp"

namespace uni {

	struct BumpArena::Block {
		Block* next;
		stduint capacity;
		stduint used;
	};

	BumpArena::BumpArena(stduint block_size)
		: root_block(nullptr), current_block(nullptr), tail_block(nullptr),
		default_block_size(block_size ? block_size : 0x1000), used_size(0),
		capacity_size(0), block_count(0) {
	}

	BumpArena::~BumpArena() {
		Clear();
	}

	BumpArena::BumpArena(BumpArena&& other) noexcept
		: root_block(other.root_block), current_block(other.current_block),
		tail_block(other.tail_block),
		default_block_size(other.default_block_size),
		used_size(other.used_size), capacity_size(other.capacity_size),
		block_count(other.block_count) {
		other.root_block = nullptr;
		other.current_block = nullptr;
		other.tail_block = nullptr;
		other.used_size = 0;
		other.capacity_size = 0;
		other.block_count = 0;
	}

	BumpArena& BumpArena::operator=(BumpArena&& other) noexcept {
		if (this == &other) return *this;

		Clear();
		root_block = other.root_block;
		current_block = other.current_block;
		tail_block = other.tail_block;
		default_block_size = other.default_block_size;
		used_size = other.used_size;
		capacity_size = other.capacity_size;
		block_count = other.block_count;

		other.root_block = nullptr;
		other.current_block = nullptr;
		other.tail_block = nullptr;
		other.used_size = 0;
		other.capacity_size = 0;
		other.block_count = 0;
		return *this;
	}

	bool BumpArena::AddSize(stduint left, stduint right, stduint& result) {
		if (left > ~_IMM0 - right) return false;
		result = left + right;
		return true;
	}

	bool BumpArena::DecodeScale(stduint exponent, stduint& scale) {
		if (exponent >= bitsof(stduint)) return false;
		scale = _IMM1 << exponent;
		return true;
	}

	bool BumpArena::RequiredCapacity(stduint size, stduint alignment,
		stduint boundary, stduint& capacity) {
		stduint alignment_size;
		if (!DecodeScale(alignment, alignment_size)) return false;

		stduint boundary_size = 0;
		if (boundary) {
			if (!DecodeScale(boundary, boundary_size)) return false;
			if (size > boundary_size) return false;
			if (boundary <= alignment) boundary_size = 0;
		}

		capacity = size;
		if (!AddSize(capacity, alignment_size - 1, capacity)) return false;
		if (boundary_size &&
			!AddSize(capacity, boundary_size - 1, capacity)) return false;
		return true;
	}

	void* BumpArena::AllocateFrom(Block* block, stduint size,
		stduint alignment, stduint boundary, stduint& consumed) {
		stduint alignment_size;
		if (!DecodeScale(alignment, alignment_size)) return nullptr;

		stduint boundary_size = 0;
		if (boundary) {
			if (!DecodeScale(boundary, boundary_size)) return nullptr;
			if (size > boundary_size) return nullptr;
			if (boundary <= alignment) boundary_size = 0;
		}

		byte* payload = (byte*)(block + 1);
		const stduint base = _IMM(payload);
		stduint address;
		if (!AddSize(base, block->used, address)) return nullptr;

		const stduint alignment_mask = alignment_size - 1;
		if (!AddSize(address, alignment_mask, address)) return nullptr;
		address &= ~alignment_mask;

		if (boundary_size) {
			const stduint boundary_offset = address & (boundary_size - 1);
			if (boundary_offset > boundary_size - size) {
				if (!AddSize(address, boundary_size - boundary_offset,
					address)) return nullptr;
			}
		}

		if (address < base) return nullptr;
		const stduint offset = address - base;
		if (offset > block->capacity || size > block->capacity - offset)
			return nullptr;

		stduint next_used;
		if (!AddSize(offset, size, next_used)) return nullptr;
		consumed = next_used - block->used;
		block->used = next_used;
		return (void*)address;
	}

	bool BumpArena::AppendBlock(stduint minimum_capacity) {
		const stduint capacity = minimum_capacity > default_block_size ?
			minimum_capacity : default_block_size;
		stduint allocation_size;
		if (!AddSize(sizeof(Block), capacity, allocation_size)) return false;
		stduint next_capacity_size;
		if (!AddSize(capacity_size, capacity, next_capacity_size)) return false;
		if (block_count == ~_IMM0) return false;

		Block* block = (Block*)malc(allocation_size);
		if (!block) return false;
		block->next = nullptr;
		block->capacity = capacity;
		block->used = 0;

		if (tail_block) tail_block->next = block;
		else root_block = block;
		tail_block = block;
		current_block = block;
		capacity_size = next_capacity_size;
		++block_count;
		return true;
	}

	void* BumpArena::allocate(stduint size, stduint alignment,
		stduint boundary) {
		if (!size) size = 1;

		stduint minimum_capacity;
		if (!RequiredCapacity(size, alignment, boundary, minimum_capacity))
			return nullptr;

		Block* block = current_block ? current_block : root_block;
		while (block) {
			stduint consumed = 0;
			if (void* result = AllocateFrom(block, size, alignment, boundary,
				consumed)) {
				current_block = block;
				used_size += consumed;
				return result;
			}
			block = block->next;
		}

		if (!AppendBlock(minimum_capacity)) return nullptr;
		stduint consumed = 0;
		void* result = AllocateFrom(current_block, size, alignment, boundary,
			consumed);
		if (result) used_size += consumed;
		return result;
	}

	bool BumpArena::deallocate(void* ptr, stduint size) {
		(void)ptr;
		(void)size;
		return false;
	}

	void* BumpArena::reallocate(void* ptr, stduint old_size, stduint size,
		stduint alignment) {
		void* result = allocate(size, alignment);
		if (!result) return nullptr;

		const stduint copy_size = old_size < size ? old_size : size;
		if (ptr && copy_size) MemCopyN(result, ptr, copy_size);
		return result;
	}

	void* BumpArena::AllocateZero(stduint size, stduint alignment,
		stduint boundary) {
		void* result = allocate(size, alignment, boundary);
		if (result) MemSet(result, 0, size ? size : 1);
		return result;
	}

	char* BumpArena::CopyString(const char* string, stduint length) {
		if (!string) return nullptr;
		if (!length) length = StrLength(string);

		stduint allocation_size;
		if (!AddSize(length, 1, allocation_size)) return nullptr;
		char* result = (char*)allocate(allocation_size);
		if (!result) return nullptr;
		if (length) MemCopyN(result, string, length);
		result[length] = 0;
		return result;
	}

	void BumpArena::Reset() {
		for (Block* block = root_block; block; block = block->next)
			block->used = 0;
		current_block = root_block;
		used_size = 0;
	}

	void BumpArena::Clear() {
		Block* block = root_block;
		while (block) {
			Block* next = block->next;
			memf(block);
			block = next;
		}
		root_block = nullptr;
		current_block = nullptr;
		tail_block = nullptr;
		used_size = 0;
		capacity_size = 0;
		block_count = 0;
	}

}
