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
	PGPROP_nonexecutable = 0x20,
	// x86 page-cache controls.  PCD | PWT selects UC with the default PAT.
	PGPROP_cache_disable = 0x40,
	PGPROP_write_through = 0x80,

	// TYPE
	PGPROP_MASK_TYPE = 0x700,
	PGTYPE_default = 0x000,
	PGTYPE_writeback = 0x100,
	PGTYPE_writethrough = 0x200,
	PGTYPE_uncacheable = 0x300,
	PGTYPE_writecombine = 0x400,
	PGTYPE_device = 0x500,
	PGTYPE_noncacheable = 0x600,

	PGOPTS_omit_warnings = 0x1000,
};

#if defined(_ARC_x86) || defined(_ARC_x64)
namespace uni {
	// The caller must perform the architecture-required multiprocessor cache and TLB synchronization protocol.
	bool EnablePagingPAT();
	bool PagingPATEnabled();
	bool PagingPhysicalAddressSupported(uint64 physical_address, stduint level);
	bool ReadX86MTRRType(uint64 physical_address, stduint& type);
}
#elif defined(_ARC_ARM_64)
namespace uni {
	// The caller must configure the same MAIR layout on every processor before sharing page tables.
	bool EnablePagingMAIR();
	bool PagingMAIREnabled();
}
#elif defined(_ARC_ARM_32)
namespace uni {
	bool PagingARM32MemoryTypesEnabled();
}
#elif defined(_ARC_RISCV_32) || defined(_ARC_RISCV_64)
namespace uni {
	void SetPagingSvpbmtSupported(bool supported);
	bool PagingSvpbmtSupported();
}
#endif

inline bool NormalizePageProperties(stduint prop, stduint& normalized) {
	const stduint legacy = prop & (PGPROP_cache_disable | PGPROP_write_through);
	stduint type = prop & PGPROP_MASK_TYPE;
	if (type == PGPROP_MASK_TYPE || (type != PGTYPE_default && legacy)) return false;
	if (type == PGTYPE_default && legacy) {
		if (legacy == PGPROP_write_through) type = PGTYPE_writethrough;
		else if (legacy == PGPROP_cache_disable) type = PGTYPE_noncacheable;
		else type = PGTYPE_uncacheable;
	}
	normalized = (prop & ~(PGPROP_MASK_TYPE |
		PGPROP_cache_disable | PGPROP_write_through)) | type;
	return true;
}

inline bool PageTypeSupported(stduint type) {
	#if defined(_ARC_x86) || defined(_ARC_x64)
	return type == PGTYPE_default || type == PGTYPE_writeback ||
		type == PGTYPE_writethrough || type == PGTYPE_uncacheable ||
		type == PGTYPE_device || type == PGTYPE_noncacheable ||
		(type == PGTYPE_writecombine && uni::PagingPATEnabled());
	#elif defined(_ARC_ARM_32)
	return uni::PagingARM32MemoryTypesEnabled() &&
		(type == PGTYPE_default || type == PGTYPE_writeback ||
		type == PGTYPE_writethrough || type == PGTYPE_uncacheable ||
		type == PGTYPE_device || type == PGTYPE_noncacheable ||
		type == PGTYPE_writecombine);
	#elif defined(_ARC_ARM_64)
	return type == PGTYPE_default || (uni::PagingMAIREnabled() &&
		(type == PGTYPE_writeback || type == PGTYPE_writethrough ||
		type == PGTYPE_uncacheable || type == PGTYPE_device ||
		type == PGTYPE_noncacheable || type == PGTYPE_writecombine));
	#elif defined(_ARC_RISCV_32)
	return type == PGTYPE_default || type == PGTYPE_writeback;
	#elif defined(_ARC_RISCV_64)
	return type == PGTYPE_default || type == PGTYPE_writeback ||
		(uni::PagingSvpbmtSupported() &&
		(type == PGTYPE_uncacheable || type == PGTYPE_device ||
		type == PGTYPE_noncacheable || type == PGTYPE_writecombine));
	#else
	return false;
	#endif
}

enum PageSizeShift : stduint {
	PAGESIZE_4KB = 12,
	PAGESIZE_1MB = 20,// ARM32
	PAGESIZE_2MB = 21,// normal 64b
	PAGESIZE_4MB = 22,// normal 32b
	PAGESIZE_1GB = 30,
	PAGESIZE_ANY = 0xFFFF
};

// 1 [ none   | l1p-id | l0p-id    ] until Commit 6f9dbef7
// 2 [ l1p-id | l0p-id | crt-level ] since 20260119
#if defined(_ARC_x86) || defined(_ARC_x64)
#ifdef _DEV_MSVC
#pragma pack(push, 1)
#endif
namespace uni {
	#if defined(_ARC_x64)
	bool EnablePagingNX();
	bool PagingNXEnabled();
	#endif

	_PACKED(union) PageEntry {
		stduint raw;
		_PACKED(struct) {
			stduint present : 1; // P
			stduint writable : 1;// R_W
			stduint user_access : 1;  // U_S
			stduint write_through : 1;// PWT
			stduint cache_disable : 1;// PCD
			stduint accessed : 1; // A
			stduint dirty : 1;    // D
			stduint pat : 1;
			stduint global : 1;   // G
			stduint available : 3;// AVL
			stduint address : 5 * sizeof(stduint);
			#if defined(_ARC_x64)
			stduint reserved : 11;
			stduint nonexecutable : 1;
			#endif
		} l0p_entry;
		_PACKED(struct) {
			stduint present : 1;
			stduint writable : 1;
			stduint user_access : 1;
			stduint write_through : 1;
			stduint cache_disable : 1;
			stduint accessed : 1;
			stduint ignored_6 : 1;
			stduint reserved_7 : 1;
			stduint ignored_8 : 1;
			stduint available : 3;
			stduint address : 5 * sizeof(stduint);
			#if defined(_ARC_x64)
			stduint reserved : 11;
			stduint execute_disable : 1;
			#endif
		} table_entry;
		#if defined(_ARC_x86)
		_PACKED(struct) {
			stduint present : 1;
			stduint writable : 1;
			stduint user_access : 1;
			stduint write_through : 1;
			stduint cache_disable : 1;
			stduint accessed : 1;
			stduint dirty : 1;
			stduint page_size : 1;
			stduint global : 1;
			stduint available : 3;
			stduint pat : 1;
			stduint address_high : 4;
			stduint reserved : 5;
			stduint address : 10;
		} l1p_entry;
		#elif defined(_ARC_x64)
		_PACKED(struct) {
			stduint present : 1;
			stduint writable : 1;
			stduint user_access : 1;
			stduint write_through : 1;
			stduint cache_disable : 1;
			stduint accessed : 1;
			stduint dirty : 1;
			stduint page_size : 1;
			stduint global : 1;
			stduint available : 3;
			stduint pat : 1;
			stduint reserved_low : 8;
			stduint address : 31;
			stduint reserved_high : 11;
			stduint nonexecutable : 1;
		} l1p_entry;
		_PACKED(struct) {
			stduint present : 1;
			stduint writable : 1;
			stduint user_access : 1;
			stduint write_through : 1;
			stduint cache_disable : 1;
			stduint accessed : 1;
			stduint dirty : 1;
			stduint page_size : 1;
			stduint global : 1;
			stduint available : 3;
			stduint pat : 1;
			stduint reserved_low : 17;
			stduint address : 22;
			stduint reserved_high : 11;
			stduint nonexecutable : 1;
		} l2p_entry;
		#endif

		#ifdef _INC_CPP
		inline bool isPresent() const { return l0p_entry.present; }
		inline bool isHuge(stduint level) const {
			#if defined(_ARC_x86)
			if (level != 1 || !isPresent()) return false;
			#elif defined(_ARC_x64)
			if (!level || level > 2 || !isPresent()) return false;
			#endif
			return !!(raw & (_IMM1 << 7));
		}
		inline bool isLeaf(stduint level) const {
			return isPresent() && (!level || isHuge(level));
		}
		inline bool isTable(stduint level) const {
			return level && isPresent() && !(raw & (_IMM1 << 7));
		}
		inline uint64 getAddress(stduint level) const {
			if (!level) return uint64(l0p_entry.address) << 12;
			if (!isHuge(level)) return uint64(table_entry.address) << 12;
			#if defined(_ARC_x86)
			return (uint64(l1p_entry.address) << 22) |
				(uint64(l1p_entry.address_high) << 32);
			#elif defined(_ARC_x64)
			if (level == 1) return uint64(l1p_entry.address) << 21;
			if (level == 2) return uint64(l2p_entry.address) << 30;
			return 0;
			#endif
		}
		inline bool isWritable(stduint) const { return l0p_entry.writable; }
		inline bool isUserAccessible(stduint) const { return l0p_entry.user_access; }
		inline bool DecodeLeafProperties(stduint level, stduint& prop) const {
			#if defined(_ARC_x86)
			if (level > 1) return false;
			#elif defined(_ARC_x64)
			if (level > 2) return false;
			#endif
			if (!isLeaf(level)) return false;
			prop = PGPROP_present;
			if (l0p_entry.writable) prop |= PGPROP_writable;
			if (l0p_entry.user_access) prop |= PGPROP_user_access;
			if (l0p_entry.global) prop |= PGPROP_global;
			#if defined(_ARC_x64)
			if (raw & (_IMM1 << 63)) prop |= PGPROP_nonexecutable;
			#endif
			const stduint pat = level ? !!(raw & (_IMM1 << 12)) : l0p_entry.pat;
			const stduint pat_index = (pat << 2) |
				(stduint(l0p_entry.cache_disable) << 1) | l0p_entry.write_through;
			if (pat && !PagingPATEnabled()) return false;
			switch (pat_index) {
			case 0: prop |= PGTYPE_writeback; break;
			case 1: prop |= PGTYPE_writethrough; break;
			case 2: prop |= PGTYPE_noncacheable; break;
			case 3: prop |= PGTYPE_uncacheable; break;
			case 4: prop |= PGTYPE_writecombine; break;
			case 5: prop |= PGTYPE_writethrough; break;
			case 6: prop |= PGTYPE_noncacheable; break;
			case 7: prop |= PGTYPE_uncacheable; break;
			}
			return true;
		}
		inline void Clear() { raw = 0; }
		inline bool SetupAsTable(void* next_table_ptr, stduint level) {
			if (!level || (_IMM(next_table_ptr) & 0xFFF)) return false;
			Clear();
			table_entry.address = _IMM(next_table_ptr) >> 12;
			table_entry.present = 1;
			table_entry.writable = 1;
			table_entry.user_access = 1;
			return true;
		}
		inline bool SetupAsLeaf(uint64 paddr, stduint level, stduint prop) {
			stduint shift = PAGESIZE_4KB;
			#if defined(_ARC_x86)
			if (level > 1) return false;
			if (level == 1) shift = PAGESIZE_4MB;
			#elif defined(_ARC_x64)
			if (level > 2) return false;
			if (level == 1) shift = PAGESIZE_2MB;
			else if (level == 2) shift = PAGESIZE_1GB;
			#endif
			if (paddr & ((uint64(1) << shift) - 1) ||
				!PagingPhysicalAddressSupported(paddr, level)) return false;
			stduint normalized;
			if (!NormalizePageProperties(prop, normalized)) return false;
			
			#if defined(_ARC_x86)
			if (normalized & PGPROP_nonexecutable) return false;
			#elif defined(_ARC_x64)
			if ((normalized & PGPROP_nonexecutable) && !PagingNXEnabled()) return false;
			#endif

			bool write_through = false;
			bool cache_disable = false;
			bool pat = false;
			switch (normalized & PGPROP_MASK_TYPE) {
			case PGTYPE_default:
			case PGTYPE_writeback:
				break;
			case PGTYPE_writethrough:
				write_through = true;
				break;
			case PGTYPE_uncacheable:
			case PGTYPE_device:
				write_through = cache_disable = true;
				break;
			case PGTYPE_noncacheable:
				cache_disable = true;
				break;
			case PGTYPE_writecombine:
				if (!PagingPATEnabled()) return false;
				pat = true;
				break;
			default:
				return false;
			}
			Clear();
			if (!level) l0p_entry.address = paddr >> 12;
			#if defined(_ARC_x86)
			else {
				l1p_entry.address = paddr >> 22;
				l1p_entry.address_high = paddr >> 32;
			}
			#elif defined(_ARC_x64)
			else if (level == 1) l1p_entry.address = paddr >> 21;
			else l2p_entry.address = paddr >> 30;
			#endif
			l0p_entry.present = 1;
			if (level) raw |= _IMM1 << 7;
			l0p_entry.writable = !!(normalized & PGPROP_writable);
			l0p_entry.user_access = !!(normalized & PGPROP_user_access);
			l0p_entry.global = !!(normalized & PGPROP_global);
			l0p_entry.write_through = write_through;
			l0p_entry.cache_disable = cache_disable;
			if (pat) {
				if (level) raw |= _IMM1 << 12;
				else l0p_entry.pat = 1;
			}
			#if defined(_ARC_x64)
			if (PagingNXEnabled() && (normalized & PGPROP_nonexecutable)) raw |= _IMM1 << 63;
			#endif
			return true;
		}
		#endif
	};
	static_assert(sizeof(PageEntry) == sizeof(stduint), "invalid x86 page entry size");
}
#ifdef _DEV_MSVC
#pragma pack(pop)
#endif
#elif defined(_ARC_RISCV_32) || defined(_ARC_RISCV_64)
namespace uni {
	_PACKED(union) PageEntry {
		stduint raw;
		_PACKED(struct) {
			stduint valid : 1;     // V
			stduint read : 1;      // R
			stduint writable : 1;  // W
			stduint execute : 1;   // X
			stduint user_access : 1;// U
			stduint global : 1;    // G
			stduint accessed : 1;  // A
			stduint dirty : 1;     // D
			stduint rsw : 2;       // reserved for OS (e.g. COW)
			stduint ppn : (sizeof(stduint) == 4 ? 22 : 44); // phyical page number
			#if defined(_ARC_RISCV_64)
			stduint reserved : 7;
			stduint pbmt : 2;
			stduint napot : 1;
			#endif
		} l0p_entry;
		_PACKED(struct) {
			stduint valid : 1;
			stduint read : 1;
			stduint writable : 1;
			stduint execute : 1;
			stduint user_access : 1;
			stduint global : 1;
			stduint accessed : 1;
			stduint dirty : 1;
			stduint rsw : 2;
			stduint ppn : (sizeof(stduint) == 4 ? 22 : 44);
			#if defined(_ARC_RISCV_64)
			stduint reserved : 10;
			#endif
		} table_entry;

		#ifdef _INC_CPP
		inline bool isPresent() const {
			return l0p_entry.valid && !(l0p_entry.writable && !l0p_entry.read);
		}
		inline bool isHuge(stduint level) const {
			return level && isPresent() &&
				(l0p_entry.read || l0p_entry.writable || l0p_entry.execute);
		}
		inline bool isLeaf(stduint) const {
			return isPresent() &&
				(l0p_entry.read || l0p_entry.writable || l0p_entry.execute);
		}
		inline bool isTable(stduint level) const {
			return level && isPresent() &&
				!(l0p_entry.read || l0p_entry.writable || l0p_entry.execute);
		}
		inline stduint getAddress(stduint) const { return _IMM(l0p_entry.ppn) << 12; }
		inline bool isWritable(stduint) const { return l0p_entry.writable; }
		inline bool isUserAccessible(stduint) const { return l0p_entry.user_access; }
		inline bool DecodeLeafProperties(stduint level, stduint& prop) const {
			#if defined(_ARC_RISCV_32)
			if (level > 1) return false;
			#elif defined(_ARC_RISCV_64)
			if (level > 2) return false;
			#endif
			if (!isLeaf(level)) return false;
			prop = PGPROP_present;
			if (l0p_entry.writable) prop |= PGPROP_writable;
			if (l0p_entry.user_access) prop |= PGPROP_user_access;
			if (l0p_entry.global) prop |= PGPROP_global;
			if (!l0p_entry.execute) prop |= PGPROP_nonexecutable;
			#if defined(_ARC_RISCV_64)
			if (l0p_entry.pbmt && !PagingSvpbmtSupported()) return false;
			switch (l0p_entry.pbmt) {
			case 0: prop |= PGTYPE_default; break;
			case 1: prop |= PGTYPE_noncacheable; break;
			case 2: prop |= PGTYPE_device; break;
			default: return false;
			}
			#else
			prop |= PGTYPE_default;
			#endif
			return true;
		}
		inline void Clear() { raw = 0; }

		inline bool SetupAsTable(void* next_table_ptr, stduint level) {
			if (!level || (_IMM(next_table_ptr) & 0xFFF)) return false;
			Clear();
			table_entry.ppn = _IMM(next_table_ptr) >> 12;
			table_entry.valid = 1;
			table_entry.read = table_entry.writable = table_entry.execute = 0;
			return true;
		}

		inline bool SetupAsLeaf(stduint paddr, stduint level, stduint prop) {
			#if defined(_ARC_RISCV_32)
			if (level > 1) return false;
			stduint shift = level ? PAGESIZE_4MB : PAGESIZE_4KB;
			#elif defined(_ARC_RISCV_64)
			if (level > 2) return false;
			stduint shift = level ?
				(level == 1 ? PAGESIZE_2MB : PAGESIZE_1GB) : PAGESIZE_4KB;
			#endif
			if (paddr & ((_IMM1 << shift) - 1)) return false;
			stduint normalized;
			if (!NormalizePageProperties(prop, normalized)) return false;
			#if defined(_ARC_RISCV_64)
			stduint pbmt = 0;
			#endif
			switch (normalized & PGPROP_MASK_TYPE) {
			case PGTYPE_default:
			case PGTYPE_writeback:
				break;
			#if defined(_ARC_RISCV_64)
			case PGTYPE_noncacheable:
			case PGTYPE_writecombine:
				if (!PagingSvpbmtSupported()) return false;
				pbmt = 1;
				break;
			case PGTYPE_uncacheable:
			case PGTYPE_device:
				if (!PagingSvpbmtSupported()) return false;
				pbmt = 2;
				break;
			#endif
			default:
				return false;
			}
			Clear();
			l0p_entry.ppn = _IMM(paddr) >> 12;
			#if defined(_ARC_RISCV_64)
			l0p_entry.pbmt = pbmt;
			#endif
			l0p_entry.valid = 1;
			l0p_entry.read = 1;
			l0p_entry.execute = !(normalized & PGPROP_nonexecutable);
			l0p_entry.writable = !!(normalized & PGPROP_writable);
			l0p_entry.user_access = !!(normalized & PGPROP_user_access);
			l0p_entry.global = !!(normalized & PGPROP_global);

			l0p_entry.accessed = 1;
			l0p_entry.dirty = !!(normalized & PGPROP_writable);
			return true;
		}
		#endif
	};
	static_assert(sizeof(PageEntry) == sizeof(stduint), "invalid RISC-V page entry size");
}
#elif defined(_ARC_ARM_64)
namespace uni {
	_PACKED(union) PageEntry {
		stduint raw;
		_PACKED(struct) {
			stduint valid : 1;      // Bit 0: Valid
			stduint is_table : 1;   // Bit 1: 1=Table/Leaf, 0=Block
			stduint attr_index : 3; // Bit 2-4: MAIR
			stduint ns : 1;         // Bit 5: Non-secure
			stduint ap : 2;         // Bit 6-7: Access Permission
			stduint sh : 2;         // Bit 8-9: Shareability
			stduint af : 1;         // Bit 10: Access Flag
			stduint ng : 1;         // Bit 11: Not Global
			stduint address : 36;   // Bit 12-47: Physical Addr
			stduint reserved_48_50 : 3;
			stduint dbm : 1;
			stduint contig : 1;
			stduint pxn : 1;
			stduint uxn : 1;
			stduint soft : 4;
			stduint reserved_59_63 : 5;
		} l0p_entry;
		_PACKED(struct) {
			stduint valid : 1;
			stduint is_table : 1;
			stduint ignored_low : 10;
			stduint address : 36;
			stduint reserved : 11;
			stduint pxn_table : 1;
			stduint xn_table : 1;
			stduint ap_table : 2;
			stduint ns_table : 1;
		} table_entry;
		_PACKED(struct) {
			stduint valid : 1;
			stduint is_table : 1;
			stduint attr_index : 3;
			stduint ns : 1;
			stduint ap : 2;
			stduint sh : 2;
			stduint af : 1;
			stduint ng : 1;
			stduint reserved_low : 9;
			stduint address : 27;
			stduint reserved_48_50 : 3;
			stduint dbm : 1;
			stduint contig : 1;
			stduint pxn : 1;
			stduint uxn : 1;
			stduint soft : 4;
			stduint reserved_59_63 : 5;
		} l1p_entry;
		_PACKED(struct) {
			stduint valid : 1;
			stduint is_table : 1;
			stduint attr_index : 3;
			stduint ns : 1;
			stduint ap : 2;
			stduint sh : 2;
			stduint af : 1;
			stduint ng : 1;
			stduint reserved_low : 18;
			stduint address : 18;
			stduint reserved_48_50 : 3;
			stduint dbm : 1;
			stduint contig : 1;
			stduint pxn : 1;
			stduint uxn : 1;
			stduint soft : 4;
			stduint reserved_59_63 : 5;
		} l2p_entry;

		#ifdef _INC_CPP
		inline bool isPresent() const { return l0p_entry.valid; }
		inline bool isHuge(stduint level) const {
			return level && level < 3 && l0p_entry.valid && !l0p_entry.is_table;
		}
		inline bool isLeaf(stduint level) const {
			return isPresent() && (level ? isHuge(level) : l0p_entry.is_table);
		}
		inline bool isTable(stduint level) const {
			return level && l0p_entry.valid && l0p_entry.is_table;
		}
		inline stduint getAddress(stduint level) const {
			if (!level) return _IMM(l0p_entry.address) << 12;
			if (isHuge(level)) {
				if (level == 1) return _IMM(l1p_entry.address) << 21;
				if (level == 2) return _IMM(l2p_entry.address) << 30;
			}
			return _IMM(table_entry.address) << 12;
		}
		inline bool isWritable(stduint) const { return !(l0p_entry.ap & 0b10); }
		inline bool isUserAccessible(stduint) const { return !!(l0p_entry.ap & 0b01); }
		inline bool DecodeLeafProperties(stduint level, stduint& prop) const {
			if (level > 2 || !isLeaf(level)) return false;
			if (l0p_entry.pxn != l0p_entry.uxn) return false;
			stduint type;
			if (!PagingMAIREnabled()) {
				if (l0p_entry.attr_index) return false;
				type = PGTYPE_default;
			}
			else {
				switch (l0p_entry.attr_index) {
				case 0: type = PGTYPE_writeback; break;
				case 1: type = PGTYPE_device; break;
				case 2: type = PGTYPE_writethrough; break;
				case 3: type = PGTYPE_noncacheable; break;
				default: return false;
				}
			}
			prop = PGPROP_present | type;
			if (isWritable(level)) prop |= PGPROP_writable;
			if (isUserAccessible(level)) prop |= PGPROP_user_access;
			if (!l0p_entry.ng) prop |= PGPROP_global;
			if (l0p_entry.pxn) prop |= PGPROP_nonexecutable;
			return true;
		}
		inline void Clear() { raw = 0; }

		inline bool SetupAsTable(void* next_table_ptr, stduint level) {
			if (!level || (_IMM(next_table_ptr) & 0xFFF)) return false;
			Clear();
			table_entry.address = _IMM(next_table_ptr) >> 12;
			table_entry.valid = 1;
			table_entry.is_table = 1;
			return true;
		}

		inline bool SetupAsLeaf(stduint paddr, stduint level, stduint prop) {
			if (level > 2) return false;
			stduint shift = level ?
				(level == 1 ? PAGESIZE_2MB : PAGESIZE_1GB) : PAGESIZE_4KB;
			if (paddr & ((_IMM1 << shift) - 1)) return false;
			stduint normalized;
			if (!NormalizePageProperties(prop, normalized)) return false;
			stduint attr_index = 0;
			stduint shareability = 0b11;
			switch (normalized & PGPROP_MASK_TYPE) {
			case PGTYPE_default:
				break;
			case PGTYPE_writeback:
				if (!PagingMAIREnabled()) return false;
				break;
			case PGTYPE_uncacheable:
			case PGTYPE_device:
				if (!PagingMAIREnabled()) return false;
				attr_index = 1;
				shareability = 0b10;
				break;
			case PGTYPE_writethrough:
				if (!PagingMAIREnabled()) return false;
				attr_index = 2;
				break;
			case PGTYPE_noncacheable:
			case PGTYPE_writecombine:
				if (!PagingMAIREnabled()) return false;
				attr_index = 3;
				break;
			default:
				return false;
			}
			Clear();
			if (!level) l0p_entry.address = _IMM(paddr) >> 12;
			else if (level == 1) l1p_entry.address = _IMM(paddr) >> 21;
			else l2p_entry.address = _IMM(paddr) >> 30;
			l0p_entry.valid = 1;
			l0p_entry.is_table = !level;
			l0p_entry.attr_index = attr_index;
			l0p_entry.sh = shareability;
			l0p_entry.af = 1;
			if (normalized & PGPROP_user_access) {
				l0p_entry.ap = (normalized & PGPROP_writable) ? 0b01 : 0b11;
			}
			else {
				l0p_entry.ap = (normalized & PGPROP_writable) ? 0b00 : 0b10;
			}
			l0p_entry.ng = !(normalized & PGPROP_global);
			if (normalized & PGPROP_nonexecutable) l0p_entry.pxn = l0p_entry.uxn = 1;
			return true;
		}
		#endif
	};
	static_assert(sizeof(PageEntry) == sizeof(stduint), "invalid ARM64 page entry size");
}
#elif defined(_ARC_ARM_32)
namespace uni {
	_PACKED(union) PageEntry {
		stduint raw;
		_PACKED(struct) {
			stduint xn : 1;         // Execute Never
			stduint type : 1;       // Small Page
			stduint b : 1;          // Bufferable
			stduint c : 1;          // Cacheable
			stduint ap : 2;         // Access Permission
			stduint tex : 3;
			stduint apx : 1;
			stduint s : 1;
			stduint ng : 1;
			stduint address : 20;   // Physical Addr (High 20 bits)
		} l0p_entry;
		_PACKED(struct) {
			stduint type : 2;       // Coarse Table
			stduint pxn : 1;
			stduint ns : 1;
			stduint reserved : 1;
			stduint domain : 4;     // Domain
			stduint implementation : 1;
			stduint address : 22;
		} l1p_table_entry;
		_PACKED(struct) {
			stduint type : 2;       // Section
			stduint b : 1;          // Bufferable
			stduint c : 1;          // Cacheable
			stduint xn : 1;         // Execute Never
			stduint domain : 4;     // Domain
			stduint p : 1;          // ECC / Present
			stduint ap : 2;         // Access Permission
			stduint tex : 3;
			stduint apx : 1;
			stduint s : 1;
			stduint ng : 1;
			stduint supersection : 1;
			stduint ns : 1;
			stduint address : 12;
		} l1p_entry;

		#ifdef _INC_CPP
		inline bool isPresent() const { return (raw & 0b11) != 0b00; }
		inline bool isHuge(stduint level) const {
			return level == 1 && l1p_entry.type == 0b10 && !l1p_entry.supersection;
		}
		inline bool isLeaf(stduint level) const {
			return level ? isHuge(level) : isPresent() && l0p_entry.type;
		}
		inline bool isTable(stduint level) const {
			return level == 1 && l1p_table_entry.type == 0b01;
		}
		inline stduint getAddress(stduint level) const {
			if (!level) return _IMM(l0p_entry.address) << 12;
			if (isHuge(level)) return _IMM(l1p_entry.address) << 20;
			return _IMM(l1p_table_entry.address) << 10;
		}
		inline bool isWritable(stduint level) const {
			return level ? !l1p_entry.apx : !l0p_entry.apx;
		}
		inline bool isUserAccessible(stduint level) const {
			return level ? l1p_entry.ap == 0b11 : l0p_entry.ap == 0b11;
		}
		inline bool DecodeLeafProperties(stduint level, stduint& prop) const {
			if (level > 1 || !isLeaf(level) || !PagingARM32MemoryTypesEnabled()) return false;
			prop = PGPROP_present;
			if (isWritable(level)) prop |= PGPROP_writable;
			if (isUserAccessible(level)) prop |= PGPROP_user_access;
			const stduint ng = level ? l1p_entry.ng : l0p_entry.ng;
			const stduint xn = level ? l1p_entry.xn : l0p_entry.xn;
			const stduint tex = level ? l1p_entry.tex : l0p_entry.tex;
			const stduint c = level ? l1p_entry.c : l0p_entry.c;
			const stduint b = level ? l1p_entry.b : l0p_entry.b;
			if (!ng) prop |= PGPROP_global;
			if (xn) prop |= PGPROP_nonexecutable;
			switch ((tex << 2) | (c << 1) | b) {
			case 0b000: prop |= PGTYPE_uncacheable; break;
			case 0b001: prop |= PGTYPE_device; break;
			case 0b010: prop |= PGTYPE_writethrough; break;
			case 0b011: prop |= PGTYPE_writeback; break;
			case 0b100: prop |= PGTYPE_noncacheable; break;
			default: return false;
			}
			return true;
		}
		inline void Clear() { raw = 0; }

		inline bool SetupAsTable(void* next_table_ptr, stduint level) {
			if (level != 1 || (_IMM(next_table_ptr) & 0x3FF)) return false;
			Clear();
			l1p_table_entry.address = _IMM(next_table_ptr) >> 10;
			l1p_table_entry.type = 0b01;
			l1p_table_entry.domain = 0;
			return true;
		}

		inline bool SetupAsLeaf(stduint paddr, stduint level, stduint prop) {
			if (level > 1) return false;
			stduint shift = level ? PAGESIZE_1MB : PAGESIZE_4KB;
			if (paddr & ((_IMM1 << shift) - 1)) return false;
			stduint normalized;
			if (!NormalizePageProperties(prop, normalized) ||
				!PagingARM32MemoryTypesEnabled()) return false;
			stduint tex = 0;
			stduint c = 0;
			stduint b = 0;
			switch (normalized & PGPROP_MASK_TYPE) {
			case PGTYPE_default:
			case PGTYPE_writeback:
				c = b = 1;
				break;
			case PGTYPE_writethrough:
				c = 1;
				break;
			case PGTYPE_uncacheable:
				break;
			case PGTYPE_device:
				b = 1;
				break;
			case PGTYPE_noncacheable:
			case PGTYPE_writecombine:
				tex = 1;
				break;
			default:
				return false;
			}
			Clear();
			if (level) {
				l1p_entry.address = paddr >> 20;
				l1p_entry.type = 0b10;
				l1p_entry.xn = !!(normalized & PGPROP_nonexecutable);
				l1p_entry.ap = (normalized & PGPROP_user_access) ? 0b11 : 0b01;
				l1p_entry.apx = !(normalized & PGPROP_writable);
				l1p_entry.ng = !(normalized & PGPROP_global);
				l1p_entry.s = 1;
				l1p_entry.tex = tex;
				l1p_entry.c = c;
				l1p_entry.b = b;
			}
			else {
				l0p_entry.address = paddr >> 12;
				l0p_entry.type = 1;
				l0p_entry.xn = !!(normalized & PGPROP_nonexecutable);
				l0p_entry.ap = (normalized & PGPROP_user_access) ? 0b11 : 0b01;
				l0p_entry.apx = !(normalized & PGPROP_writable);
				l0p_entry.ng = !(normalized & PGPROP_global);
				l0p_entry.s = 1;
				l0p_entry.tex = tex;
				l0p_entry.c = c;
				l0p_entry.b = b;
			}
			return true;
		}
		#endif
	};
	static_assert(sizeof(PageEntry) == sizeof(stduint), "invalid ARM32 page entry size");
}
#else
namespace uni {
	_PACKED(struct) PageEntry {
		int TODO;
	};
}

#endif


#if defined(_ARC_x86) || defined(_ARC_x64) ||\
	defined(_ARC_RISCV_32) || defined(_ARC_RISCV_64) || defined(_ARC_ARM_64)
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
		#elif defined(_ARC_x64)|| defined(_ARC_ARM_64)
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
		stduint crt_level : 12;
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

		// Return the entry at p.crt_level, or ~0 if the path stops earlier.
		PageEntry* refEntry(pageint p);

		// Return the mapped leaf, or ~0 for unmapped or malformed entries.
		PageEntry* getEntry(stduint address) const;
		// Write the mapped leaf level to p.crt_level.
		PageEntry* getEntry(pageint& p) const;

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
		// Inn: p provides virtual-address indexes and target_level limits the walk
		// OUT : entry points to the stop entry and p.crt_level reports its level; true means an empty entry, leaf, or target level was reached, false means the root, level, or descriptor is invalid; no page - table entry is allocated or modified.
		auto Walk(pageint & p, stduint target_level, PageEntry * &entry) const -> bool;

	public:
		static bool isSupportedSize(stduint exponent);// e.g. 12 for 4K page
	public:
		#if defined(_ARC_RISCV_32) || defined(_ARC_RISCV_64)
		inline stduint MakeSATP() {
			if (!root_level_page) return 0;
			stduint ppn = _IMM(root_level_page) >> 12;
			#if defined(_ARC_RISCV_32)
			return (1ULL << 31) | ppn; // Sv32 Mode
			#elif defined(_ARC_RISCV_64)
			return (8ULL << 60) | ppn; // Sv39 Mode
			#endif
			};
		#endif
	};

	// Memory Copy by page
	// return the data moved
	extern "C" stduint MemCopyP(void* dest, Paging& pg_d, const void* sors, Paging& pg_s, size_t n);
	extern "C" stduint StrCopyP(char* dest, Paging& pg_d, const char* sors, Paging& pg_s, size_t length);

#endif

}

#endif
