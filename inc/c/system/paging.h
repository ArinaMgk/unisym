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

enum {
	PGPROP_present = 0b1,
	PGPROP_writable = 0b10,
	PGPROP_user_access = 0b100,
	//
	PGPROP_global = 0b1000,
	PGPROP_weak = 0x10,
};

enum PageSizeShift : stduint {
	PAGESIZE_4KB = 12,
	PAGESIZE_1MB = 20,// ARM32
	PAGESIZE_2MB = 21,// normal 64b
	PAGESIZE_4MB = 22,// normal 32b
	PAGESIZE_1GB = 30
};

// 1 [ none   | l1p-id | l0p-id    ] until Commit 6f9dbef7
// 2 [ l1p-id | l0p-id | crt-level ] since 20260119
#if defined(_ARC_x86) || defined(_ARC_x64)
#ifdef _DEV_MSVC
#pragma pack(push, 1)
#endif
namespace uni {
	_PACKED(struct) PageEntry {
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
		inline bool isHuge() const { return huge_page; }
		inline stduint getAddress() const { return _IMM(address) << 12; }
		inline void Clear() { MemSet(this, 0, sizeof(PageEntry)); }
		inline void SetupAsTable(void* next_table_ptr) {
			Clear();
			this->address = _IMM(next_table_ptr) >> 12;
			this->present = 1;
			this->writable = 1;
			this->user_access = 1;
		}
		inline void SetupAsLeaf(stduint paddr, bool huge, stduint prop) {
			Clear();
			this->address = _IMM(paddr) >> 12;
			this->present = 1;
			this->huge_page = huge;
			this->writable = !!(prop & PGPROP_writable);
			this->user_access = !!(prop & PGPROP_user_access);
		}
		#endif
	};
}
#ifdef _DEV_MSVC
#pragma pack(pop)
#endif
#elif defined(_ARC_RISCV_32) || defined(_ARC_RISCV_64)
namespace uni {
	_PACKED(struct) PageEntry {
		stduint valid : 1;     // V
		stduint read : 1;      // R
		stduint write : 1;     // W
		stduint execute : 1;   // X
		stduint user : 1;      // U
		stduint global : 1;    // G
		stduint accessed : 1;  // A
		stduint dirty : 1;     // D
		stduint rsw : 2;       // reserved for OS (e.g. COW)
		stduint ppn : (sizeof(stduint) == 4 ? 22 : 44); // phyical page number
		#if defined(_ARC_RISCV_64)
		stduint reserved : 10;
		#endif

		#ifdef _INC_CPP
		inline bool isPresent() const { return valid; }
		inline bool isHuge() const { return valid && (read || write || execute); }
		inline stduint getAddress() const { return _IMM(ppn) << 12; }
		inline void Clear() { MemSet(this, 0, sizeof(PageEntry)); }

		inline void SetupAsTable(void* next_table_ptr) {
			Clear();
			this->ppn = _IMM(next_table_ptr) >> 12;
			this->valid = 1;
			this->read = this->write = this->execute = 0;
		}

		inline void SetupAsLeaf(stduint paddr, bool huge, stduint prop) {
			Clear();
			this->ppn = _IMM(paddr) >> 12;
			this->valid = 1;
			this->read = 1;
			this->execute = 1;
			this->write = !!(prop & PGPROP_writable);
			this->user = !!(prop & PGPROP_user_access);
		}
		#endif
	};
}
#elif defined(_ARC_ARM_64)
namespace uni {
	_PACKED(struct) PageEntry {
		stduint valid : 1;      // Bit 0: Valid
		stduint is_table : 1;   // Bit 1: 1=Table/Leaf, 0=Block
		stduint attr_index : 3; // Bit 2-4: MAIR
		stduint ns : 1;         // Bit 5: Non-secure
		stduint ap : 2;         // Bit 6-7: Access Permission
		stduint sh : 2;         // Bit 8-9: Shareability
		stduint af : 1;         // Bit 10: Access Flag
		stduint ng : 1;         // Bit 11: Not Global
		stduint address : 36;   // Bit 12-47: Physical Addr
		stduint reserved : 4;
		stduint dbm : 1;
		stduint contig : 1;
		stduint pxn : 1;
		stduint uxn : 1;
		stduint soft : 8;

		#ifdef _INC_CPP
		inline bool isPresent() const { return valid; }
		inline bool isHuge() const { return valid && !is_table; }
		inline stduint getAddress() const { return _IMM(address) << 12; }
		inline void Clear() { MemSet(this, 0, sizeof(PageEntry)); }

		inline void SetupAsTable(void* next_table_ptr) {
			Clear();
			this->address = _IMM(next_table_ptr) >> 12;
			this->valid = 1;
			this->is_table = 1;
		}

		inline void SetupAsLeaf(stduint paddr, bool huge, stduint prop) {
			Clear();
			this->address = _IMM(paddr) >> 12;
			this->valid = 1;
			this->is_table = huge ? 0 : 1;
			this->af = 1;
			if (prop & PGPROP_user_access) {
				this->ap = (prop & PGPROP_writable) ? 0b01 : 0b11;
			}
			else {
				this->ap = (prop & PGPROP_writable) ? 0b00 : 0b10;
			}
		}
		#endif
	};
}
#elif defined(_ARC_ARM_32)
namespace uni {
	_PACKED(struct) PageEntry {
		stduint type : 2;       // 00=Fault, 01=Table, 10=Section/Page
		stduint b : 1;          // Bufferable
		stduint c : 1;          // Cacheable
		stduint xn : 1;         // Execute Never
		stduint domain : 4;     // Domain
		stduint p : 1;          // ECC / Present
		stduint ap : 2;         // Access Permission
		stduint address : 20;   // Physical Addr (High 20 bits)

		#ifdef _INC_CPP
		inline bool isPresent() const { return type != 0b00; }
		inline bool isHuge() const { return type == 0b10; }
		inline stduint getAddress() const { return _IMM(this) & 0xFFFFFC00; }
		inline void Clear() { MemSet(this, 0, sizeof(PageEntry)); }

		inline void SetupAsTable(void* next_table_ptr) {
			Clear();
			treat<stduint>(this) = (_IMM(next_table_ptr) & 0xFFFFFC00) | 0b01;
			this->domain = 0;
		}

		inline void SetupAsLeaf(stduint paddr, bool huge, stduint prop) {
			Clear();
			if (huge) {
				treat<stduint>(this) = (paddr & 0xFFF00000) | 0b10;
			}
			else {
				treat<stduint>(this) = (paddr & 0xFFFFF000) | 0b10;
				this->c = 1; this->b = 1;
			}
			this->ap = (prop & PGPROP_user_access) ? 0b11 : 0b10;
		}
		#endif
	};
}
#else
namespace uni {
	_PACKED(struct) PageEntry {
		int TODO;
	};
}

#endif


#if defined(_ARC_x86) || defined(_ARC_x64) ||\
	defined(_ARC_RISCV_32) || defined(_ARC_RISCV_64) || defined(_ARC_ARM64)
namespace uni {
	_PACKED(struct) pageint {
		stduint crt_level : 12;
		#if defined(_ARC_x86) || defined(_ARC_RISCV_32)
		stduint l0p_index : 10;// PT
		stduint l1p_index : 10;// PD, the entries contains the pgsize
		#elif defined(_ARC_RISCV_64)
		stduint l0p_index : 9;
		stduint l1p_index : 9;
		stduint l2p_index : 9;
		stduint pg_size : 25;// expo, 12 or 21 or 30
		#elif defined(_ARC_x64)|| defined(_ARC_ARM64)
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
#elif defined(_ARC_ARM_32)
namespace uni {
	_PACKED(struct) pageint {
		stduint offset : 12;
		stduint l0p_index : 8;  // Level 2 (PT)
		stduint l1p_index : 12; // Level 1 (PD)
		pageint(stduint address) { treat<stduint>(this) = address; }
		operator stduint() { return treat<stduint>(this); }
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

		~Paging();

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

		auto getPageSizeShift(stduint address) const -> stduint;
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
