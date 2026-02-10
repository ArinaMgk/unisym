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

bool SinglePool::ifContainAny(const Slice& slice) {
	auto crtpool = this;
	while (crtpool && crtpool->slicecnt) {
		Slice* p = &crtpool->slices[0];
		for0(i, crtpool->slicecnt) {
			if (p->address >= slice.address + slice.length) return false;
			if (p->address + p->length > slice.address)
				return true;
			p++;
		}
		crtpool = crtpool->nextpool;
	}
	return false;
}


pureptr_t SinglePool::Locate(stduint idx) const {
	stduint level = idx / CNT_SLICES_PER_POOL;
	stduint index = idx % CNT_SLICES_PER_POOL;
	auto crtpool = this;
	while (level > 0 && crtpool->nextpool) {
		crtpool = crtpool->nextpool;
		level--;
	}
	if (level || crtpool->slicecnt <= index) return nullptr;
	return pureptr_t(&crtpool->slices[index]);
}
stduint   SinglePool::Locate(pureptr_t p_val, bool fromRight) const {
	_TODO
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
	_Comment(unused) return false;
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

void* Mempool::allocate(stduint size, stduint alignment) {
	for0(i, pool_available.Length()) {
		_TODO;
	}
}
bool Mempool::deallocate(void* ptr, stduint size _Comment(zero_for_block)) {
	_TODO
}
