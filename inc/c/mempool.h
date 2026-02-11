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

extern
::uni::trait::Malloc* uni_default_allocator;
namespace uni {
	const unsigned CNT_SLICES_PER_POOL = (0x1000 / sizeof(Slice)) - 3;
	_PACKED(struct) SinglePool
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
		//
	public:
		Mempool() {}
		void Reset(const Slice& slice) {
			new (&pool_registerd) SinglePool();
			new (&pool_available) SinglePool();
			pool_registerd.Append(slice);
			pool_available.Append(slice);
		}

	public:// trait
		virtual void* allocate(stduint size, stduint alignment = 0) override;
		virtual bool deallocate(void* ptr, stduint size = 0 _Comment(zero_for_block)) override;

	};// store virtual addresses

} //END C++ Area
#else//: C Area

//END C Area
#endif
#endif
