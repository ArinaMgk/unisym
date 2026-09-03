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

#ifndef _INC_MEMPOOL
#define _INC_MEMPOOL

#include "./stdinc.h"

#if defined(_INC_CPP)
#include "../cpp/trait/ArrayTrait.hpp"
#include "../cpp/trait/MallocTrait.hpp"
extern "C" {
#endif


#if defined(_INC_CPP)
} //: C++ Area

extern ::uni::trait::Malloc
* uni_default_allocator,
* uni_hostenv_allocator;

namespace uni {
	const unsigned CNT_SLICES_PER_POOL = (0x1000 / sizeof(Slice)) - 3;
	// ---- Page-based allocation (4 KiB, no header) ----
	// A sparse radix tree (PageMetaNode) records every 4 KiB page by page
	// number (address >> 12): inner entries hold child-node addresses, leaf
	// entries hold page status FREE (0) / CONT (~0, continuation) / a run head
	// storing its page count. A run stores the count at its head page and CONT
	// on the rest, so deallocate only needs the head pointer. Node pages are
	// carved from pool_available and owned via the parent pointer, never
	// tracked as user pages; levels follow MEMPOOL_ADDR_BITS (4 on 64-bit).
	const unsigned MEMPOOL_PAGE_SHIFT = 12;
	const stduint MEMPOOL_PAGE_SIZE = (stduint)1 << MEMPOOL_PAGE_SHIFT;
	const unsigned PAGE_META_ENTRY_COUNT = 0x1000 / sizeof(stduint);
	const unsigned PAGE_META_LEVEL_BITS = sizeof(stduint) == 8 ? 9 : 10;
	// effective address width; on 64-bit hosts only the low 48 bits are used
	// (canonical sign extension is ignored), giving a 36-bit page number
	const unsigned MEMPOOL_ADDR_BITS = sizeof(stduint) == 8 ? 48 : 32;
	const unsigned PAGE_META_LEVELS =
		(MEMPOOL_ADDR_BITS - MEMPOOL_PAGE_SHIFT + PAGE_META_LEVEL_BITS - 1)
		/ PAGE_META_LEVEL_BITS;
	constexpr stduint PAGE_META_FREE = 0;
	constexpr stduint PAGE_META_CONT = ~_IMM0;
	// one node covers one 4 KiB page. leaf entries hold page status,
	// inner entries hold the address of the child node (0 for none).
	struct PageMetaNode {
		stduint entries[PAGE_META_ENTRY_COUNT];
	};
	static_assert(sizeof(PageMetaNode) == MEMPOOL_PAGE_SIZE,
		"PageMetaNode does not fit one page");
	struct SinglePool
		: public trait::Array
	{
		SinglePool* nextpool;
		SinglePool* leftpool;
		stduint owner_id;
		stduint slicecnt;// for this pool
		Slice slices[CNT_SLICES_PER_POOL];// ordered
		// ---- The first pool using ----
		// return the id of the slice, -1 for failure. if all part of slice exists in the pool
		stdsint ifContainAll(const Slice& slice);
		// if any part of slice exists in the pool. return the id of the slice if need inserting, -1 for contain any.
		stdsint ifContainNon(const Slice& slice);
		//
		bool Append(const Slice& slice);
		//
		bool Remove(const Slice& slice);
	public:// trait
		virtual pureptr_t Locate(stduint idx) const override;
		virtual stduint   Locate(pureptr_t p_val, bool fromRight) const override;
		virtual stduint   Length() const override;
		virtual bool      Insert(stduint idx, pureptr_t dat) override;
		virtual bool      Remove(stduint idx, stduint times) override;
		virtual bool      Exchange(stduint idx1, stduint idx2) override;
		inline Slice* operator[](stduint idx) { return (Slice*)Locate(idx); }
	};
	
	class Mempool : public trait::Malloc {
		SinglePool pool_registerd = {};
		SinglePool pool_available = {};
		PageMetaNode* page_root = nullptr;// sparse page-table root for page allocation
		//
	public:
		bool enable_auto_expand = true;
		stduint auto_expand_step = 0x40000;
		bool expand_only_from_self = false;
	public:
		Mempool() {}
		void Reset() {
			new (&pool_registerd) SinglePool();
			new (&pool_available) SinglePool();
			page_root = nullptr;// drop the sparse page table; its pages come back with the slice
			if (expand_only_from_self) {
				pool_registerd.owner_id = 1;
				pool_available.owner_id = 1;
			}
		}
		void Reset(const Slice& slice) {
			Reset();
			pool_registerd.Append(slice);
			pool_available.Append(slice);
		}

		void Append(const Slice& slice) {
			if (expand_only_from_self) {
				pool_registerd.owner_id = 1;
				pool_available.owner_id = 1;
			}
			pool_registerd.Append(slice);
			pool_available.Append(slice);
		}

		bool Expand(stduint min_size = 0);

	public:// trait
		virtual void* allocate(stduint size, stduint alignment = 0, stduint boundary = 0) override;
		virtual bool deallocate(void* ptr, stduint size = 0 _Comment(zero_for_block)) override;

	public:// page allocation (4 KiB granularity, no header inside)
		void* allocate_pages(stduint page_count, stduint alignment = MEMPOOL_PAGE_SHIFT);
		bool deallocate_pages(void* ptr);

	private:// page-table helpers
		PageMetaNode* allocate_meta_page();
		bool deallocate_meta_page(PageMetaNode* node);
		bool ensure_page_root();
		stduint* get_leaf_entry(stduint page_no, bool create);
		void free_page_table(PageMetaNode* node, stduint level);

	public:// debug
		void dump_available();
	};// store virtual addresses

} //END C++ Area
#else//: C Area

//END C Area
#endif
#endif
