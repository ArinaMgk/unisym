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
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

#include "../../../inc/c/mempool.h"

#if !defined(SET_POISON) && defined(_DEBUG)
#define SET_POISON 1
#endif
#if !defined(SET_POISON)
#define SET_POISON 0
#endif

using namespace uni;

::uni::trait::Malloc* uni_default_allocator
= nullptr;
::uni::trait::Malloc* uni_hostenv_allocator
= nullptr;

static bool PrepareNextPool(SinglePool* root, SinglePool* tail)
{
	const stduint pool_size = 0x1000;

	static_assert(
		sizeof(SinglePool) <= pool_size,
		"SinglePool exceeds one page");

	SinglePool* new_pool = nullptr;
	stduint new_pool_addr = 0;

	if (root->owner_id == 1) {
		SinglePool* crtpool = root;
		while (crtpool && crtpool->slicecnt) {
			for0(i, crtpool->slicecnt) {
				Slice* p = &crtpool->slices[i];

				if (p->length < pool_size)
					continue;

				stduint addr =
					ceilAlign(pool_size, p->address);

				if (addr + pool_size <=
					(stduint)p->getEndoaddr())
				{
					new_pool_addr = addr;
					new_pool = (SinglePool*)addr;
					break;
				}
			}

			if (new_pool)
				break;

			crtpool = crtpool->nextpool;
		}
	}
	else {
		if (!uni_default_allocator) {
			plogerro(
				"uni_default_allocator is nullptr");

			new_pool = (SinglePool*)
				malloc(sizeof(SinglePool));
		}
		else {
			new_pool = (SinglePool*)
				uni_default_allocator->allocate(
					sizeof(SinglePool), 12);
		}
	}

	if (!new_pool)
		return false;

	new (new_pool) SinglePool();

	new_pool->nextpool = nullptr;
	new_pool->leftpool = tail;
	new_pool->owner_id = root->owner_id;
	new_pool->slicecnt = 0;

	tail->nextpool = new_pool;

	if (root->owner_id == 1) {
		if (!root->Remove(
			Slice{ new_pool_addr, pool_size }))
		{
			tail->nextpool = nullptr;
			return false;
		}
	}

	return true;
}

stdsint SinglePool::ifContainAll(const Slice& slice) {
	stduint lev = 0;
	auto crtpool = this;
	while (crtpool && crtpool->slicecnt) {
		Slice* p = &crtpool->slices[0];
		for0(i, crtpool->slicecnt) {
			if (p->address > slice.address) return -1;
			if (p->address + p->length >= slice.address + slice.length)
				return lev * CNT_SLICES_PER_POOL + i;
			p++;
		}
		crtpool = crtpool->nextpool;
		lev++;
	}
	return -1;
}

stdsint SinglePool::ifContainNon(const Slice& slice) {
	stduint lev = 0;
	auto crtpool = this;
	while (crtpool && crtpool->slicecnt) {
		Slice* p = &crtpool->slices[0];
		for0(i, crtpool->slicecnt) {
			if (p->address >= slice.address + slice.length) return lev * CNT_SLICES_PER_POOL + i;
			if (p->address + p->length > slice.address)
				return -1;
			p++;
		}
		crtpool = crtpool->nextpool;
		lev++;
	}
	return Length();
}

bool SinglePool::Append(const Slice& slice) {
	auto idx = ifContainNon(slice);
	if (idx == -1) return false;
	return Insert(idx, (pureptr_t)&slice);
}

bool SinglePool::Remove(const Slice& slice) {
	// e.g. remove {1,2} from {0,10} -> {0,1}&{3,7}
	//{FUTURE} for R3APP, left&unmap page if throw a pool
	auto idx = ifContainAll(slice);
	if (-1 == idx) return false;
	stduint level = idx / CNT_SLICES_PER_POOL;
	stduint index = idx % CNT_SLICES_PER_POOL;
	auto crtpool = this;
	while (level > 0 && crtpool->nextpool) {
		crtpool = crtpool->nextpool;
		level--;
	}
	auto parent = (&crtpool->slices[index]);
	const stduint parent_end = parent->address + parent->length;
	const stduint slice_end  = slice.address + slice.length;
	const stduint left_len  = slice.address - parent->address;
	const stduint right_len = parent_end - slice_end;
	if (left_len == 0 && right_len == 0) {
		return Remove(idx, 1);
	}
	else if (left_len > 0 && right_len == 0) {
		// Case remove {2,8} from {0,10} -> {0,2}
		parent->length = left_len;
		return true;
	}
	else if (left_len == 0 && right_len > 0) {
		// Case remove {0,2} from {0,10} -> {2,8}
		parent->address = slice_end;
		parent->length  = right_len;
		return true;
	}
	else {
		// Case remove {1,2} from {0,10}
		parent->length = left_len;
		Slice new_right_part;
		new_right_part.address = slice_end;
		new_right_part.length  = right_len;
		return Insert(idx + 1, &new_right_part); 
	}
}

pureptr_t SinglePool::Locate(stduint idx) const {
	stduint level = idx / CNT_SLICES_PER_POOL;
	stduint index = idx % CNT_SLICES_PER_POOL;
	auto crtpool = this;
	while (level > 0 && crtpool->nextpool) {
		crtpool = crtpool->nextpool;
		level--;
	}
	if (!crtpool || level || crtpool->slicecnt <= index) return nullptr;
	return pureptr_t(&crtpool->slices[index]);
}
stduint   SinglePool::Locate(pureptr_t p_val, bool fromRight) const {
	_TODO return ~_IMM0;
}
stduint   SinglePool::Length() const {
	stduint ret = 0;
	auto crtpool = this;
	while (crtpool->nextpool) {
		if (!crtpool->slicecnt) return ret;
		ret += crtpool->slicecnt;// may equal CNT_SLICES_PER_POOL
		crtpool = crtpool->nextpool;
	}
	ret += crtpool->slicecnt;
	return ret;
}
bool      SinglePool::Insert(stduint idx, pureptr_t dat _Comment(pointer to a slice)) {
	if (!dat) return false;
	Slice src = *(Slice*)dat;
	//
	stduint total_len;
	SinglePool* last_pool;
	while (true) {
		total_len = Length();
		if (idx > total_len) return false;
		//
		// see left
		if (idx > 0) {
			auto& left = *(Slice*)Locate(idx - 1);
			if (left.address + left.length == src.address) {
				left.length += src.length;
				if (auto p = (Slice*)Locate(idx)) {
					if (left.address + left.length == p->address) {
						left.length += p->length;
						return Remove(idx, 1);
					}
				}
				return true;
			}
		}
		// see right
		if (auto p = (Slice*)Locate(idx)) {
			if (src.address + src.length == p->address) {
				p->address = src.address;
				p->length += src.length;
				return true;
			}
		}
		//
		SinglePool* physical_tail = this;
		while (physical_tail->nextpool)
			physical_tail = physical_tail->nextpool;

		// Reserve one slot for splitting the storage slice,
		// and another slot for the current insertion.
		if (physical_tail->slicecnt >= CNT_SLICES_PER_POOL - 2) {
			if (!PrepareNextPool(this, physical_tail))
				return false;

			// PrepareNextPool may change the slice array.
			stdsint new_idx = ifContainNon(src);
			if (new_idx < 0) return false;
			idx = (stduint)new_idx;
			continue;
		}
		//
		last_pool = this;
		while (last_pool->nextpool &&
			last_pool->slicecnt >= CNT_SLICES_PER_POOL)
		{
			last_pool = last_pool->nextpool;
		}
		break;
	}
	//
	auto last_pool0 = last_pool;
	stduint cnt_dst = total_len % CNT_SLICES_PER_POOL;
	if (last_pool->slicecnt == 0 && last_pool->leftpool == nullptr) {
		if (cnt_dst != 0) {
			plogerro("cnt_dst != 0");
			return false;
		}
		last_pool->slices[cnt_dst] = src;
		last_pool->slicecnt++;
		return true;
	}
	//
	stduint cnt_src = (total_len - 1) % CNT_SLICES_PER_POOL;
	SinglePool* ento_pool = last_pool->slicecnt == 0
		? last_pool->leftpool : last_pool;
	for (stduint i = total_len; i > idx; --i) {
		last_pool->slices[cnt_dst] = ento_pool->slices[cnt_src];
		cnt_dst = cnt_src;
		last_pool = ento_pool;
		if (cnt_src == 0) {
			cnt_src = CNT_SLICES_PER_POOL - 1;
			ento_pool = ento_pool->leftpool;
		}
		else cnt_src--;
	}
	last_pool->slices[cnt_dst] = src;
	last_pool0->slicecnt++;
	return true;
}
bool      SinglePool::Remove(stduint idx, stduint times) {
	if (!times) return true;
	stduint count = Length();
	if (!count || idx + times > count) return false;
	// Moving
	auto dst = self[idx];
	auto src = self[idx + times];
	SinglePool* dst_pool = (SinglePool*)(_IMM(self[floorAlign(CNT_SLICES_PER_POOL, idx)]) - offsetof(SinglePool, slices));
	SinglePool* src_pool = (SinglePool*)(_IMM(self[floorAlign(CNT_SLICES_PER_POOL, idx + times)]) - offsetof(SinglePool, slices));
	stduint cnt_dst = idx % CNT_SLICES_PER_POOL;
	stduint cnt_src = (idx + times) % CNT_SLICES_PER_POOL;
	for0(i, count - idx - times) {
		*dst = *src;
		if (++cnt_dst >= CNT_SLICES_PER_POOL) {
			cnt_dst = 0;
			dst_pool = dst_pool->nextpool;
			dst = &dst_pool->slices[0];
		}
		else dst++;
		if (++cnt_src >= CNT_SLICES_PER_POOL) {
			cnt_src = 0;
			src_pool = src_pool->nextpool;
			src = &src_pool->slices[0];
		}
		else src++;
	}
	// Update slicecnt
	dst_pool->slicecnt = cnt_dst;
	for (auto clear_ptr = dst_pool->nextpool;
		clear_ptr&& clear_ptr->slicecnt;
		clear_ptr = clear_ptr->nextpool)
	{
		clear_ptr->slicecnt = 0;	
	}
	return true;
}
bool      SinglePool::Exchange(stduint idx1, stduint idx2) {
	_Comment(unused) return false;
}

_PACKED(struct) Header {
	stduint size;
	stduint prop;
};

bool Mempool::Expand(stduint min_size) {
	if (expand_only_from_self) {
		return false;
	}
	if (!uni_default_allocator) {
		plogerro("Mempool::Expand failed: uni_default_allocator is nullptr");
		return false;
	}
	stduint req_size = maxof(min_size, auto_expand_step);
	req_size = ceilAlign(auto_expand_step, req_size);
	void* raw_mem = uni_default_allocator->allocate(req_size);
	if (!raw_mem) {
		req_size = maxof(min_size, auto_expand_step);
		req_size = ceilAlign(0x1000, req_size);
		raw_mem = uni_default_allocator->allocate(req_size);
	}
	if (!raw_mem) {
		plogerro("Mempool::Expand failed: Host environment OOM (%u bytes)", req_size);
		return false;
	}
	Slice new_slice = { _IMM(raw_mem), req_size };
	this->Append(new_slice);
	#if 0 && !defined(_ACCM)////
	printlog(_LOG_TRACE, "Mempool Expanded by %u bytes at 0x%[x]", req_size, _IMM(raw_mem));
	#endif
	return true;
}

void* Mempool::allocate(stduint size, stduint alignment, stduint boundary) {
	stduint align = _IMM1 << alignment;
	if (boundary <= alignment) boundary = nil;
	stduint bound = boundary ? (_IMM1 << boundary) : 0;
	// Conforming to C/C++: return a unique valid pointer for 0-byte allocations

	if (!size) size = 1;
	// if (!size) return nullptr;

	// page-aligned requests whose size is a whole number of pages use the
	// headerless page allocator; bound == 0 means no crossing constraint remains
	if (alignment >= MEMPOOL_PAGE_SHIFT && bound == 0 && (size % MEMPOOL_PAGE_SIZE) == 0) {
		stduint page_count = size >> MEMPOOL_PAGE_SHIFT;
		return allocate_pages(page_count, alignment);
	}

	if (bound > 0 && size > bound) return nullptr;
	const stduint total_size = sizeof(Header) + size;
	int retry_count = 0;
_RETRY_ALLOC:
	auto crtpool = &pool_available;
	while (crtpool && crtpool->slicecnt) {
		for0(i, crtpool->slicecnt) {
			Slice* p = &crtpool->slices[i];
			if (p->length < total_size) {
				continue;
			}
			Header* header;
			pureptr_t ret;
			stduint next_align = ceilAlign(align, p->address + sizeof(Header));
			//
			if (bound > 1) {
				stduint mask = ~(bound - 1);
				if ((next_align & mask) != ((next_align + size - 1) & mask)) {
					next_align = (next_align & mask) + bound;// over bound
				}
			}
			//
			if (align <= 1) {
				if (next_align == p->address + sizeof(Header)) {
					header = (Header*)p->address;
					ret = (void*)_IMM(header + 1);
				}
				else if (next_align + size <= (stduint)p->getEndoaddr()) {
					header = (Header*)(next_align - sizeof(Header));
					ret = (void*)next_align;
				}
				else continue;
			}
			else if (next_align + size <= (stduint)p->getEndoaddr()) {
				header = (Header*)(next_align - sizeof(Header));
				ret = (void*)next_align;
			}
			else {
				continue;
			}
			header->size = size;
			header->prop = _IMM(0xFEDC5AA5);
			auto succ = pool_available.Remove(Slice{ _IMM(header), total_size });
			if (!succ) {
				plogerro("Remove failed in Mempool::allocate %u a%u b%u", size, alignment, boundary);
				plogerro("\t Remove(Slice{ %[x], %[x] })", _IMM(header), total_size);
				plogerro("\t Current Slice{ %[x], %[x] }", p->address, p->length);
			}
			// if (1) MemSet(ret, 0, size);
			// ploginfo("Mempool::allocate %u a%u b%u -> %[x]", size, alignment, boundary, ret);
			return ret;
		}
		crtpool = crtpool->nextpool;
	}
	if (enable_auto_expand && retry_count == 0) {
		retry_count++;
		stduint worst_padding = (bound > align ? bound : align);
		stduint safe_request = total_size + worst_padding;
		if (Expand(safe_request)) {
			goto _RETRY_ALLOC;
		}
	}

	return nullptr;
}
bool Mempool::deallocate(void* ptr, stduint size _Comment(zero_for_block)) {
	// ploginfo("Mempool::deallocate %p s%u", ptr, size);
	// page allocations carry no header; detect them by 4 KiB alignment + page table
	if (ptr && ((_IMM(ptr) & (MEMPOOL_PAGE_SIZE - 1)) == 0)) {
		stduint* entry = get_leaf_entry(_IMM(ptr) >> MEMPOOL_PAGE_SHIFT, false);
		if (entry && *entry != PAGE_META_FREE) {
			return deallocate_pages(ptr);
		}
	}
	if (_IMM(ptr) < sizeof(Header)) return false;

	Header* header = (Header*)ptr - 1;
	if (header->prop != _IMM(0xFEDC5AA5) || !header->size) {
		plogerro("Mempool::deallocate, %[x]\n\rsiz%[x] pro%[32H]",
			ptr, header->size, header->prop);
		return false;
	}
	if (!size) size = header->size;
	#if SET_POISON
	MemSet(ptr, 0xDD, size); // poison freed memory to detect use-after-free
	#endif
	Slice recovered = { _IMM(header) , sizeof(Header) + size };
	bool state;
	if (size == header->size) {
		state = pool_available.Append(recovered);
	}
	else {
		plogwarn("Partial deallocation %u/%u", size, header->size);
		// moving the header before the rest of the block
		stduint dealloc_size = minof(header->size, size);
		stduint rest_data_addr = (stduint)ptr + dealloc_size;
		Header* new_header = (Header*)rest_data_addr - 1;
		new_header->size = header->size - dealloc_size;
		new_header->prop = _IMM(0xFEDC5AA5);
		recovered.length = dealloc_size;
		state = pool_available.Append(recovered);
	}
	return state;
}

// ---- Page-based allocation (4 KiB, no header) ----
// take an aligned 4 KiB page from pool_available for a page-table node;
// the page is not tracked in the page table itself (owned via parent pointer)
PageMetaNode* Mempool::allocate_meta_page() {
	for (auto crtpool = &pool_available; crtpool; crtpool = crtpool->nextpool) {
		for0(i, crtpool->slicecnt) {
			Slice* p = &crtpool->slices[i];
			if (p->length < MEMPOOL_PAGE_SIZE) continue;
			stduint addr = ceilAlign(MEMPOOL_PAGE_SIZE, p->address);
			if (addr + MEMPOOL_PAGE_SIZE > (stduint)p->getEndoaddr()) continue;
			if (!pool_available.Remove(Slice{ addr, MEMPOOL_PAGE_SIZE })) continue;
			PageMetaNode* node = (PageMetaNode*)addr;
			MemSet(node, 0, MEMPOOL_PAGE_SIZE);
			return node;
		}
	}
	return nullptr;
}

bool Mempool::deallocate_meta_page(PageMetaNode* node) {
	if (!node) return false;
	return pool_available.Append(Slice{ _IMM(node), MEMPOOL_PAGE_SIZE });
}

bool Mempool::ensure_page_root() {
	if (page_root) return true;
	page_root = allocate_meta_page();
	return page_root != nullptr;
}

// walk the sparse page table down to the leaf entry of a page number;
// create missing nodes only when create is true (deallocate stays read-only)
stduint* Mempool::get_leaf_entry(stduint page_no, bool create) {
	if (!page_root) return nullptr;
	PageMetaNode* node = page_root;
	for (stduint level = PAGE_META_LEVELS - 1; level >= 1; --level) {
		stduint idx = (page_no >> (level * PAGE_META_LEVEL_BITS)) & (PAGE_META_ENTRY_COUNT - 1);
		PageMetaNode* child = (PageMetaNode*)node->entries[idx];
		if (!child) {
			if (!create) return nullptr;
			child = allocate_meta_page();
			if (!child) return nullptr;
			node->entries[idx] = _IMM(child);
		}
		node = child;
	}
	return &node->entries[page_no & (PAGE_META_ENTRY_COUNT - 1)];
}

// reserved for on-the-fly empty-node reclaim (later phase); not called yet
void Mempool::free_page_table(PageMetaNode* node, stduint level) {
	if (!node) return;
	if (level >= 1) {
		for0(i, PAGE_META_ENTRY_COUNT) {
			PageMetaNode* child = (PageMetaNode*)node->entries[i];
			if (child) free_page_table(child, level - 1);
		}
	}
	deallocate_meta_page(node);
}

void* Mempool::allocate_pages(stduint page_count, stduint alignment) {
	if (!page_count || page_count == PAGE_META_CONT) return nullptr;
	if (page_count > ~_IMM0 / MEMPOOL_PAGE_SIZE) return nullptr;// size would wrap
	if (alignment < MEMPOOL_PAGE_SHIFT) alignment = MEMPOOL_PAGE_SHIFT;// 4 KiB is the floor

	const stduint align = _IMM1 << alignment;
	const stduint size = page_count * MEMPOOL_PAGE_SIZE;

	// one allocation attempt; returns nullptr when the pool cannot satisfy it
	auto attempt_alloc = [&]() -> void* {
		if (!ensure_page_root()) return nullptr;

		// locate an aligned free run large enough
		stduint addr = 0;
		for (auto crtpool = &pool_available; crtpool && !addr; crtpool = crtpool->nextpool) {
			for0(i, crtpool->slicecnt) {
				Slice* p = &crtpool->slices[i];
				stduint aligned = ceilAlign(align, p->address);
				if (aligned + size <= (stduint)p->getEndoaddr()) {
					addr = aligned;
					break;
				}
			}
		}
		if (!addr) return nullptr;

		// reserve the user region first so page-table nodes never take pages from it
		if (!pool_available.Remove(Slice{ addr, size })) return nullptr;

		// build every page-table path and confirm all leaf entries are free
		const stduint first_page = addr >> MEMPOOL_PAGE_SHIFT;
		for (stduint p = 0; p < page_count; ++p) {
			stduint* entry = get_leaf_entry(first_page + p, true);
			if (!entry || *entry != PAGE_META_FREE) {
				pool_available.Append(Slice{ addr, size });// roll back; keep built empty nodes
				return nullptr;
			}
		}

		// commit: first entry holds the page count, the rest are continuations
		for (stduint p = 0; p < page_count; ++p) {
			*get_leaf_entry(first_page + p, false) = p ? PAGE_META_CONT : page_count;
		}
		return (void*)addr;
	};

	void* result = attempt_alloc();
	if (!result && enable_auto_expand && Expand(size + align)) {
		result = attempt_alloc();
	}
	return result;
}

bool Mempool::deallocate_pages(void* ptr) {
	if (!ptr) return false;
	stduint address = _IMM(ptr);
	if (address & (MEMPOOL_PAGE_SIZE - 1)) return false;// not 4 KiB aligned
	if (!page_root) return false;

	const stduint first_page = address >> MEMPOOL_PAGE_SHIFT;
	stduint* first_entry = get_leaf_entry(first_page, false);
	if (!first_entry) return false;// not owned by the page allocator
	if (*first_entry == PAGE_META_FREE || *first_entry == PAGE_META_CONT) return false;

	const stduint page_count = *first_entry;
	if (page_count > ~_IMM0 / MEMPOOL_PAGE_SIZE) return false;// damaged entry

	// verify the whole run before touching anything
	for (stduint p = 1; p < page_count; ++p) {
		stduint* entry = get_leaf_entry(first_page + p, false);
		if (!entry || *entry != PAGE_META_CONT) return false;
	}

	// hand the run back, then clear the entries
	if (!pool_available.Append(Slice{ address, page_count * MEMPOOL_PAGE_SIZE })) return false;
	for (stduint p = 0; p < page_count; ++p) {
		*get_leaf_entry(first_page + p, false) = PAGE_META_FREE;
	}
	return true;
}

void Mempool::dump_available() {
	ploginfo("=== Mempool Available Slices Report ===");
	stduint total_free = 0;
	stduint slice_index = 0;
	SinglePool* crtpool = &pool_available;

	while (crtpool) {
		for (stduint i = 0; i < crtpool->slicecnt; ++i) {
			Slice* s = &crtpool->slices[i];
			ploginfo("  [%u] 0x%[x] -> 0x%[x] | Size: 0x%[x] (%u bytes)", 
				slice_index++, 
				s->address, 
				s->address + s->length, 
				s->length, 
				s->length
			);
			total_free += s->length;
		}
		crtpool = crtpool->nextpool;
	}

	if (slice_index == 0) {
		plogwarn("  (No available memory slices!)");
	}

	ploginfo("  Summary: %u slices, Total free: %u bytes", slice_index, total_free);
	ploginfo("========================================");
}
