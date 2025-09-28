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

#if !defined(_INC_System_Paging) && defined(_INC_CPP)
#define _INC_System_Paging
// #include "../bitmap.h"

namespace uni {

	struct Page;
	

#if defined(_ARC_x86)

#ifdef _DEV_MSVC
#pragma pack(push, 1)
	struct
#else
	_PACKED(struct)
#endif
	 PageEntry {
		stduint P : 1;
		stduint R_W : 1;
		stduint U_S : 1;
		stduint PWT : 1;
		stduint PCD : 1;
		stduint A : 1;
		stduint D : 1;
		stduint PAT : 1;
		stduint G : 1;
		stduint AVL : 3;
		stduint address : 20;
	};// both page table and page directory

#ifdef _DEV_MSVC
#pragma pack(pop)
#else
	
#endif

// 0 .. 0x400
#define _NUM_pg_table_entries  (0x1000 / byteof(dword))
#define _NUM_pd_table_entries  (0x1000 / byteof(dword))

	struct Paging;

	struct PageTable;
	struct PageDirectory;
	struct Page {
		PageEntry* getEntry(Paging& pg2);
		// return virtual linear address
		operator stduint() const {
			return _IMM(this) << 12;
		}
		//
		PageTable& getParent() const;
		bool isPresent(Paging& pg2) const;
		inline stduint getID() const {
			return _IMM(this) & 0x3FF;// 0x003FF000
		}
	};

	// below are virtual entity

	extern void(*(*_physical_allocate)(stduint size));
	struct PageTable {
		PageEntry* Index(Paging& pg2);
		Page& operator[](stduint pg_id) const {
			if (pg_id >= _NUM_pg_table_entries)
				return *(Page*)~_IMM0;
			return *(Page*)((_IMM(this) << 10) | pg_id);
		}
		
		//
		PageDirectory& getParent(Paging& pg2) const;
		bool isPresent(Paging& pg2) const;
		stduint getID() const {
			return _IMM(this) & 0x3FF;// 0xFFC00000
		}
		PageEntry* getEntry(Paging& pg2);
	};// this = 0xFFC00 | id(0x000 ~ 0x3FF)

	struct PageDirectory {
		PageEntry* Index();
		PageTable& operator[](stduint pt_id) const {
			if (pt_id >= _NUM_pd_table_entries)
				return *(PageTable*)~_IMM0;
			return *(PageTable*)pt_id;
		}
	};

	struct Paging {
		PageDirectory* page_directory;


		// return phyical address, ~0 for unmapped
		void* operator[](stduint address) const;

		// return ~0 for unmapped
		PageEntry* getEntry(stduint address) const;

		inline Page* IndexPage(stduint address) {
			stduint idx_p1 = address; idx_p1 >>= 12 + 10; idx_p1 &= 0x3FF; // index of page table
			stduint idx_p0 = address; idx_p0 >>= 12; idx_p0 &= 0x3FF; // index of page
			return &(*page_directory)[idx_p1][idx_p0];
		}

		bool Map(stduint address, stduint physical_address, stduint length, bool writable, bool user_but_superv);
		bool MapWeak(stduint address, stduint physical_address, stduint length, bool writable, bool user_but_superv);

		void Reset();

		// ---- Platform Related ----

		//
		// set usual properties
		void setMode(PageTable& l1p, bool present = true, bool writable = true, bool user_but_superv = true);
		void setMode(Page& pag, bool present = true, bool writable = true, bool user_but_superv = true, stduint link_to_phy = 0);


	};

	// Memory Copy by page
	// return the data moved
	extern "C" stduint MemCopyP(void* dest, Paging& pg_d, const void* sors, Paging& pg_s, size_t n);

	
#endif

}

#endif
