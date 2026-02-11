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

using namespace uni;

::uni::trait::Malloc* uni_default_allocator
= nullptr;

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
	Slice& src = *(Slice*)dat;
	stduint total_len = Length();
	if (idx > total_len) return false;
	//
	// see left
	if (idx > 0) {
		auto& left = *(Slice*)Locate(idx - 1);
		if (left.address + left.length == src.address) {
			left.length += src.length;
			if (auto p = (Slice*)Locate(idx)) {
				if (left.address + left.length == p->address)
					return Remove(idx, 1);
			}
			return true;
		}
	}
	if (auto p = (Slice*)Locate(idx)) {
		if (src.address + src.length == p->address) {
			p->address = src.address;
			p->length += src.length;
			return true;
		}
	}
	//
	SinglePool* last_pool = this;
	while (last_pool->nextpool && last_pool->slicecnt >= CNT_SLICES_PER_POOL)
		last_pool = last_pool->nextpool;
	if (last_pool->slicecnt >= CNT_SLICES_PER_POOL) { // > is for safety, last_pool->nextpool should be nullptr
		SinglePool* new_pool = (SinglePool*)(uni_default_allocator ?
			uni_default_allocator->allocate(sizeof(SinglePool)) :
			malloc(sizeof(SinglePool)));
		if (!new_pool) return false;
		new (new_pool) SinglePool();
		last_pool->nextpool = (SinglePool*)new_pool;
		new_pool->nextpool = nullptr;
		new_pool->leftpool = last_pool;
		new_pool->slicecnt = 0;
		new_pool->owner_id = this->owner_id;
		last_pool = new_pool;
	}
	auto last_pool0 = last_pool;
	stduint cnt_dst = total_len % CNT_SLICES_PER_POOL;
	stduint cnt_src = (total_len - 1) % CNT_SLICES_PER_POOL;
	if (last_pool->slicecnt == 0 && last_pool->leftpool == nullptr) {
		if (cnt_dst != 0) {
			plogerro("cnt_dst != 0");
			return false;
		}
		last_pool->slices[cnt_dst] = src;
		last_pool->slicecnt++;
		return true;
	}
	SinglePool* ento_pool = last_pool->slicecnt == 0 ? last_pool->leftpool : last_pool;
	for (stduint i = total_len; i > idx; --i) {
		last_pool->slices[cnt_dst] = ento_pool->slices[cnt_src];
		cnt_dst = cnt_src; last_pool = ento_pool;
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

void* Mempool::allocate(stduint size, stduint alignment) {
	stduint align = _IMM1 << alignment;
	if (!size) return nullptr;
	const stduint total_size = sizeof(Header) + size;
	auto crtpool = &pool_available;
	while (crtpool && crtpool->slicecnt) {
		for0(i, crtpool->slicecnt) {
			Slice* p = &crtpool->slices[i];
			if (p->length < total_size) {
				continue;
			}
			Header* header;
			pureptr_t ret;
			const stduint next_align = ceilAlign(align, p->address + sizeof(Header));
			if (align <= 1) {
				header = (Header*)p->address;
				ret = (void*)_IMM(header + 1);
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
				plogerro("Remove failed in Mempool::allocate");
			}
			return ret;
		}
		crtpool = crtpool->nextpool;
	}
	return nullptr;
}
bool Mempool::deallocate(void* ptr, stduint size _Comment(zero_for_block)) {
	if (_IMM(ptr) < sizeof(Header)) return false;
	Header* header = (Header*)ptr - 1;
	if (header->prop != _IMM(0xFEDC5AA5) || !header->size) {
		plogerro("Invalid pointer in Mempool::deallocate");
		return false;
	}
	if (!size) size = header->size;
	Slice recovered = {_IMM(header) , sizeof(Header) + size};
	if (size == header->size) {
		return pool_available.Append(recovered);
	}
	else {
		// moving the header before the rest of the block
		stduint dealloc_size = minof(header->size, size);
		stduint rest_data_addr = (stduint)ptr + dealloc_size;
		Header* new_header = (Header*)rest_data_addr - 1;
		new_header->size = header->size - dealloc_size;
		new_header->prop = _IMM(0xFEDC5AA5);
		recovered.length = dealloc_size;
		return pool_available.Append(recovered);
	}
}
