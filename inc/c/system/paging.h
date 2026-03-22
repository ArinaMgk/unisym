// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (System) Paging
// Codifiers: @dosconio: 20241209 ~ <Last-check> 
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

#include "../stdinc.h"
#if defined(_INC_CPP)
#include "../../cpp/trait/MallocTrait.hpp"
#endif

#if !defined(_INC_System_Paging) && defined(_INC_CPP)
#define _INC_System_Paging


// x86 Lev2: 00000000~FFFFFFFF
//     [v]4KB [ ]4MB
// x64 Lev4: 000000000000~FFFFFFFFFFFF
//     [v]4KB [v]2MB [v]1GB ---

// x86 4MB
// - set CR4.PSE
// - set PDE.PAT of l1p_index

// x64 2MB/1GB
// - enable PAE
// - set PDE.PAT of l1p_index(2M) or l2p_index(1G)

// 1 [ none   | l1p-id | l0p-id    ] until Commit 6f9dbef7
// 2 [ l1p-id | l0p-id | crt-level ] since 20260119
#if defined(_ARC_x86) || defined(_ARC_x64)
namespace uni {
	#ifdef _DEV_MSVC
	#pragma pack(push, 1)
	struct
		#else
	_PACKED(struct)
		#endif
		PageEntry{
			stduint present : 1; // P
			stduint writable : 1;// R_W
			stduint user_access : 1;  // U_S
			stduint write_through : 1;// PWT
			stduint cache_disable : 1;// PCD
			stduint accessed : 1; // A
			stduint dirty : 1;    // D
			stduint huge_page : 1;// PAT
			stduint global : 1;   // G
			stduint available : 3;// AVL
			stduint address : 5 * sizeof(stduint);
			#if defined(_ARC_x64)
			stduint : 12;
			#endif
			//
	#ifdef _INC_CPP
			inline bool isPresent() const { return present; };
			inline stduint getAddress() const { return _IMM(address) << 12; }
	#endif
	};// both page table and page directory


	#ifdef _DEV_MSVC
	#pragma pack(pop)
	#else

	#endif
}
#else
namespace uni {
	_PACKED(struct) PageEntry {
		int TODO;
	};
}

#endif

enum {
	PGPROP_present = 0b1,
	PGPROP_writable = 0b10,
	PGPROP_user_access = 0b100,
	//
	PGPROP_global = 0b1000,
	PGPROP_weak = 0x10,
};

enum PageSizeShift : stduint {
	PAGESIZE_4KB  = 12,
	PAGESIZE_2MB  = 21,
	PAGESIZE_4MB  = 22,
	PAGESIZE_1GB  = 30
};


#if defined(_ARC_x86) || defined(_ARC_x64)
namespace uni {
	_PACKED(struct) pageint {
		stduint crt_level : 12;
		#if defined(_ARC_x86)
		stduint l0p_index : 10;// PT
		stduint l1p_index : 10;// PD, the entries contains the pgsize
		#else
		stduint l0p_index : 9;
		stduint l1p_index : 9;
		stduint l2p_index : 9;// PDPT
		stduint l3p_index : 9;// PML4
		stduint pg_size : 16;// expo, 12 or 21 or 30
		#endif
		//
		pageint(stduint address) {
			treat<stduint>(this) = address;
		}
		operator stduint() {
			return treat<stduint>(this);
		}
	};
}
#else
namespace uni { _PACKED(struct) pageint { stduint default_value; }; }
#endif

#if defined(_INC_CPP)
extern
::uni::trait::Malloc* uni_default_allocator;
#endif

namespace uni {

#if 1

	#if defined(_ARC_x86)
	// 0 .. 0x400
	#define _NUM_pg_table_entries  (0x1000 / byteof(dword))
	#define _NUM_pd_table_entries  (0x1000 / byteof(dword))
	extern void(*(*_physical_allocate)(stduint size));
	#endif

	struct Paging {
		PageEntry* root_level_page;

		// return ~0 for unmapped
		// do not consider huge-page
		PageEntry* refEntry(pageint p);

		// return ~0 for unmapped
		PageEntry* getEntry(stduint address) const;

		// return physical address, ~0 for unmapped
		void* operator[](stduint address) const;

		// default: writable
		auto
			Map(stduint linear_address,
				stduint physical_address,
				stduint length,
				PageSizeShift pgsize,
				stduint pgporp
			) -> bool;

		//{unchk} unmap
		auto
			Unmap(stduint ln_address, stduint length) -> bool;

		void Reset();

	protected:
		auto PageMap(stduint laddr, stduint paddr, stduint pgsize, stduint pgporp) -> bool;
	};

	// Memory Copy by page
	// return the data moved
	extern "C" stduint MemCopyP(void* dest, Paging& pg_d, const void* sors, Paging& pg_s, size_t n);
	extern "C" stduint StrCopyP(char* dest, Paging& pg_d, const char* sors, Paging& pg_s, size_t length);

#endif

}

#endif
