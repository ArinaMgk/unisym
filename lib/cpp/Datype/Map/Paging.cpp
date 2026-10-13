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

#include "../../../../inc/c/system/paging.h"
#include "../../../../inc/c/ustring.h"
#if defined(_ARC_x86) || defined(_ARC_x64)
#include "../../../../inc/c/proctrl/IAx86_64.msr.h"
#include "../../../../inc/c/proctrl/IAx86_64.h"
#endif
// a32 (unchk)

// a64 (unchk)


#if defined(_ARC_x86)
// x86 Lev2: 00000000~FFFFFFFF
//     [v]4KB [v]4MB
// [x86] 4MB
// - set CR4.PSE
// - set PDE.PAT of l1p_index
constexpr stduint PAGE_LEVELS = 2;
constexpr stduint VPN_SHIFTS[] = {12, 22};
constexpr stduint PAGE_ENTRY_COUNTS[] = {1024, 1024};
constexpr bool PAGE_LEVEL_CAN_LEAF[] = {true, true};

#elif defined(_ARC_x64)
// x64 Lev4: 000000000000~FFFFFFFFFFFF
//     [v]4KB [v]2MB [v]1GB ---
// x64 2MB/1GB
// - enable PAE
// - set PDE.PAT of l1p_index(2M) or l2p_index(1G)
constexpr stduint PAGE_LEVELS = 4;
constexpr stduint VPN_SHIFTS[] = {12, 21, 30, 39};
constexpr stduint PAGE_ENTRY_COUNTS[] = {512, 512, 512, 512};
constexpr bool PAGE_LEVEL_CAN_LEAF[] = {true, true, true, false};

#elif defined(_ARC_ARM_32)
constexpr stduint PAGE_LEVELS = 2;
constexpr stduint VPN_SHIFTS[] = {12, 20};
constexpr stduint PAGE_ENTRY_COUNTS[] = {256, 4096};
constexpr bool PAGE_LEVEL_CAN_LEAF[] = {true, true};

#elif defined(_ARC_ARM_64)
constexpr stduint PAGE_LEVELS = 4;
constexpr stduint VPN_SHIFTS[] = {12, 21, 30, 39};
constexpr stduint PAGE_ENTRY_COUNTS[] = {512, 512, 512, 512};
constexpr bool PAGE_LEVEL_CAN_LEAF[] = {true, true, true, false};

#elif defined(_ARC_RISCV_32)
// r32 Lev2: 00000000~FFFFFFFF
//     [v]4KB [v]4MB
// [RISCV]
// - if R = W = X = 0 then current table is the table of table but a page
constexpr stduint PAGE_LEVELS = 2;
constexpr stduint VPN_SHIFTS[] = {12, 22};
constexpr stduint PAGE_ENTRY_COUNTS[] = {1024, 1024};
constexpr bool PAGE_LEVEL_CAN_LEAF[] = {true, true};

#elif defined(_ARC_RISCV_64)
// r64
constexpr stduint PAGE_LEVELS = 3;
constexpr stduint VPN_SHIFTS[] = {12, 21, 30};
constexpr stduint PAGE_ENTRY_COUNTS[] = {512, 512, 512};
constexpr bool PAGE_LEVEL_CAN_LEAF[] = {true, true, true};
#endif

static_assert(sizeof(VPN_SHIFTS) / sizeof(VPN_SHIFTS[0]) == PAGE_LEVELS, "invalid paging shift count");
static_assert(sizeof(PAGE_ENTRY_COUNTS) / sizeof(PAGE_ENTRY_COUNTS[0]) == PAGE_LEVELS, "invalid paging entry count");
static_assert(sizeof(PAGE_LEVEL_CAN_LEAF) / sizeof(PAGE_LEVEL_CAN_LEAF[0]) == PAGE_LEVELS, "invalid paging leaf count");

namespace uni {
	inline stduint getVPN(stduint address, stduint level) {
		return (address >> VPN_SHIFTS[level]) & (PAGE_ENTRY_COUNTS[level] - 1);
	}

	struct PageMapWrite {
		PageEntry* target;
		stduint expected_raw;
		PageEntry value;
		stduint virtual_address;
	};

	struct PageMapTransaction {
		bool build;
		bool direct_write;
		bool any_size;
		stduint maximum_level;
		stduint properties;
		stduint table_count;
		stduint write_count;
		stduint table_index;
		stduint write_index;
		PageEntry** tables;
		PageMapWrite* writes;
		bool saw_present;
		bool saw_not_present;
		bool property_change;
		stduint property_address;
		stduint property_level;
		bool finer_structure;
		stduint finer_address;
		stduint finer_level;
	};

	static bool ExecutePageMapTransaction(PageMapTransaction& transaction,
		PageEntry* root, stduint virtual_address, uint64 physical_address,
		stduint length);
}

namespace uni {
	static stduint ComparablePageType(stduint type) {
		#if defined(_ARC_x86) || defined(_ARC_x64)
		if (type == PGTYPE_device) return PGTYPE_uncacheable;
		#elif defined(_ARC_ARM_64)
		if (type == PGTYPE_uncacheable) return PGTYPE_device;
		if (type == PGTYPE_writecombine) return PGTYPE_noncacheable;
		#elif defined(_ARC_ARM_32)
		if (type == PGTYPE_writecombine) return PGTYPE_noncacheable;
		#elif defined(_ARC_RISCV_32) || defined(_ARC_RISCV_64)
		if (type == PGTYPE_writeback) return PGTYPE_default;
		#if defined(_ARC_RISCV_64)
		if (type == PGTYPE_uncacheable) return PGTYPE_device;
		if (type == PGTYPE_writecombine) return PGTYPE_noncacheable;
		#endif
		#endif
		return type;
	}

	static bool PagePropertiesCompatible(const PageEntry& entry, stduint level, stduint requested) {
		stduint existing;
		if (!entry.DecodeLeafProperties(level, existing)) return false;
		const stduint permission_mask = PGPROP_writable | PGPROP_user_access |
			PGPROP_global | PGPROP_nonexecutable;
		if ((requested & permission_mask) != (existing & permission_mask)) return false;
		const stduint requested_type = requested & PGPROP_MASK_TYPE;
		if (requested_type != PGTYPE_default &&
			ComparablePageType(requested_type) !=
			ComparablePageType(existing & PGPROP_MASK_TYPE)) return false;
		return true;
	}

	#if defined(_ARC_ARM_32)

	Paging::~Paging() {
		if (!root_level_page || !uni_default_allocator) return;
		for (stduint i = 0; i < PAGE_ENTRY_COUNTS[1]; i++) {
			PageEntry& pde = root_level_page[i];
			if (pde.isTable(1)) {
				void* pt = (void*)pde.getAddress(1);
				uni_default_allocator->deallocate(pt, 0x1000);
			}
		}
		uni_default_allocator->deallocate(root_level_page, 0x4000);
		root_level_page = nullptr;
	}

	#else

	static void FreePageTable(PageEntry* table, int level) {
		if (!table) return;
		if (level > 0) {
			stduint max_entries = PAGE_ENTRY_COUNTS[level];
			for (stduint i = 0; i < max_entries; i++) {
				PageEntry& pe = table[i];
				if (pe.isTable(level)) {
					PageEntry* next_table = (PageEntry*)pe.getAddress(level);
					FreePageTable(next_table, level - 1);
				}
			}
		}
		if (uni_default_allocator) {
			uni_default_allocator->deallocate(table, 0x1000);
		}
	}

	Paging::~Paging() {
		if (!root_level_page || !uni_default_allocator) return;
		FreePageTable(root_level_page, PAGE_LEVELS - 1);
		root_level_page = nullptr;
	}

	#endif

	auto Paging::Walk(pageint& p, stduint target_level, PageEntry*& entry) const -> bool {
		entry = nullptr;
		if (!root_level_page || target_level >= PAGE_LEVELS) return false;

		const stduint address = (stduint)p;
		PageEntry* table = root_level_page;
		for (int level = PAGE_LEVELS - 1; level >= (int)target_level; --level) {
			entry = &table[getVPN(address, level)];
			p.crt_level = level;
			if (!entry->isPresent() || entry->isLeaf(level) || level == target_level)
				return true;
			if (!entry->isTable(level)) return false;
			table = (PageEntry*)entry->getAddress(level);
		}
		return false;
	}

	auto Paging::refEntry(pageint p) -> PageEntry* {
		const stduint target_level = p.crt_level;
		PageEntry* entry;
		if (!Walk(p, target_level, entry) || p.crt_level != target_level ||
			!entry || !entry->isPresent()) return (PageEntry*)~_IMM0;
		return entry;
	}

	auto Paging::getEntry(pageint& p) const -> PageEntry* {
		PageEntry* entry;
		if (!Walk(p, 0, entry) || !entry || !entry->isLeaf(p.crt_level))
			return (PageEntry*)~_IMM0;
		return entry;
	}

	auto Paging::getEntry(stduint address) const -> PageEntry* {
		pageint p = address;
		return getEntry(p);
	}

	auto Paging::getPageSizeShift(stduint address) const -> stduint {
		pageint p = address;
		PageEntry* entry = getEntry(p);
		if (_IMM(entry) == ~_IMM0) return 0;
		return VPN_SHIFTS[p.crt_level];
	}

	void* Paging::operator[](stduint address) const {
		pageint p = address;
		auto entry = getEntry(p);
		if (_IMM(entry) == ~_IMM0) return (void*)~_IMM0;

		stduint shift = VPN_SHIFTS[p.crt_level];
		stduint mask = (1ULL << shift) - 1;
		return (void*)(entry->getAddress(p.crt_level) + (address & mask));
	}

	void Paging::Reset() {
		stduint root_size = 0x1000;
		stduint root_align = PAGESIZE_4KB;
		#if defined(_ARC_ARM_32)
		root_size = 0x4000;
		root_align = 14;
		#endif

		if (uni_default_allocator) {
			root_level_page = (uni::PageEntry*)uni_default_allocator->allocate(root_size, root_align);
		}
		#if defined(_ARC_x86)
		else if (_physical_allocate) {
			root_level_page = (uni::PageEntry*)_physical_allocate(root_size);
		}
		#endif

		if (root_level_page) MemSet(root_level_page, 0, root_size);
	}

	auto Paging::Map(stduint ln_address, stduint ph_address, stduint length, PageSizeShift pgsize, stduint pgporp) -> bool {
		if (!length) return true;
		const bool omit_warnings = pgporp & PGOPTS_omit_warnings;
		pgporp &= ~PGOPTS_omit_warnings;
		stduint normalized;
		if (!NormalizePageProperties(pgporp, normalized) ||
			!PageTypeSupported(normalized & PGPROP_MASK_TYPE)) return false;
		pgporp = normalized;
		if (pgporp & PGPROP_present) {
			PageEntry property_probe;
			if (!property_probe.SetupAsLeaf(0, 0, pgporp) ||
				!PagePropertiesCompatible(property_probe, 0, pgporp)) return false;
		}
		const stduint requested_linear = ln_address;
		const stduint requested_length = length;
		stduint maximum_level = PAGE_LEVELS - 1;
		if (pgsize == PAGESIZE_ANY) {
			const stduint page_mask = (_IMM1 << PAGESIZE_4KB) - 1;
			const stduint virtual_offset = ln_address & page_mask;
			if (virtual_offset != (ph_address & page_mask) ||
				ln_address + length - 1 < ln_address ||
				ph_address + length - 1 < ph_address) return false;
			ln_address -= virtual_offset;
			ph_address -= virtual_offset;
			if (length > ~_IMM0 - virtual_offset) return false;
			length += virtual_offset;
			if (length > ~_IMM0 - page_mask) return false;
			length = (length + page_mask) & ~page_mask;
		}
		else {
			int target_level = -1;
			for (stduint level = 0; level < PAGE_LEVELS; level++) {
				if (PAGE_LEVEL_CAN_LEAF[level] && VPN_SHIFTS[level] == pgsize) {
					target_level = level;
					break;
				}
			}
			if (target_level < 0 || !isSupportedSize(pgsize)) return false;
			maximum_level = target_level;
			const stduint unit = _IMM1 << VPN_SHIFTS[target_level];
			const stduint alignmask = unit - 1;
			ph_address &= ~alignmask;
			if (ln_address + length - 1 < ln_address) return false;
			const stduint virtual_offset = ln_address & alignmask;
			if (length > ~_IMM0 - virtual_offset) return false;
			length += virtual_offset;
			ln_address &= ~alignmask;
			if (length > ~_IMM0 - alignmask) return false;
			length = (length + alignmask) & ~alignmask;
		}
		if (!length || ln_address + length - 1 < ln_address ||
			ph_address + length - 1 < ph_address) return false;

		PageMapTransaction transaction = {};
		transaction.any_size = pgsize == PAGESIZE_ANY;
		transaction.maximum_level = maximum_level;
		transaction.properties = pgporp;
		if (!ExecutePageMapTransaction(transaction, root_level_page,
			ln_address, ph_address, length)) return false;

		if (!omit_warnings && transaction.property_change) {
			plogwarn("Paging::Map requests changes to existing page properties at VA=%[x] level=%u",
				transaction.property_address, transaction.property_level);
		}
		if (!omit_warnings && !transaction.any_size && transaction.finer_structure) {
			plogwarn("Paging::Map explicit page size follows an existing finer page table at VA=%[x] requested-shift=%u actual-level=%u",
				transaction.finer_address, (stduint)pgsize, transaction.finer_level);
		}
		if (!omit_warnings && transaction.saw_present && transaction.saw_not_present) {
			plogwarn("Paging::Map range contains both present and non-present pages at VA=%[x] length=%[u]",
				requested_linear, requested_length);
		}
		return true;
	}

	auto Paging::Unmap(stduint ln_address, stduint length) -> bool {
		if (!length) return true;
		const stduint page_mask = (_IMM1 << PAGESIZE_4KB) - 1;
		const stduint offset = ln_address & page_mask;
		if (length > ~_IMM0 - offset) return false;
		ln_address -= offset;
		length += offset;
		return Map(ln_address, 0, length, PAGESIZE_ANY, 0);
	}


	stduint MemCopyP(void* dest, Paging& pg_d, const void* sors, Paging& pg_s, size_t length)
	{
		stduint ret = 0;

		while (length) {
			stduint phy_d, phy_s;
			stduint unit = length;

			// --- Process Destination ---

			if (pg_d.root_level_page) {
				pageint p = _IMM(dest);
				auto crt = pg_d.getEntry(p);
				if (_IMM(crt) == ~_IMM0) {
					plogerro(" at %s(dest %[x], %[x], sors %[x], %[x], length %[u])",
						__FUNCIDEN__, dest, pg_d.root_level_page, sors, pg_s.root_level_page, length
					);
					plogerro("MemCopyP: dest page is not present. rest %[u]B", length);
					return 0;
				}
				stduint shift = VPN_SHIFTS[p.crt_level];
				stduint mask = (1ULL << shift) - 1;
				stduint offset = _IMM(dest) & mask;
				phy_d = crt->getAddress(p.crt_level) + offset;
				MIN(unit, (1ULL << shift) - offset);
			}
			else {
				// Treat as raw physical address
				phy_d = _IMM(dest);
			}

			// --- Process Source ---
			if (pg_s.root_level_page) {
				pageint p = _IMM(sors);
				auto crt = pg_s.getEntry(p);
				if (_IMM(crt) == ~_IMM0) {
					plogerro("MemCopyP: sors page is not present for %[x] in %[x]", sors, pg_s.root_level_page);
					return 0;
				}
				stduint shift = VPN_SHIFTS[p.crt_level];
				stduint mask = (1ULL << shift) - 1;
				stduint offset = _IMM(sors) & mask;
				phy_s = crt->getAddress(p.crt_level) + offset;
				MIN(unit, (1ULL << shift) - offset);
			}
			else {
				// Treat as raw physical address
				phy_s = _IMM(sors);
			}
			// plogwarn("= %x %x %x", phy_d, phy_s, unit);

			MemCopyN((void*)(phy_d), (void*)(phy_s), unit);
			cast<char*>(dest) += unit;
			cast<char*>(sors) += unit;
			length -= unit;
			ret += unit;
		}
		return ret;
	}

	stduint StrCopyP(char* dest, Paging& pg_d, const char* sors, Paging& pg_s, size_t length)
	{
		stduint ret = 0;

		while (length) {
			stduint phy_d, phy_s;
			stduint unit = length;

			// --- Process Destination ---

			if (pg_d.root_level_page) {
				pageint p = _IMM(dest);
				auto crt = pg_d.getEntry(p);
				if (_IMM(crt) == ~_IMM0) return 0;
				stduint shift = VPN_SHIFTS[p.crt_level];
				stduint mask = (1ULL << shift) - 1;
				stduint offset = _IMM(dest) & mask;
				phy_d = crt->getAddress(p.crt_level) + offset;
				MIN(unit, (1ULL << shift) - offset);
			}
			else {
				phy_d = _IMM(dest);
			}

			// --- Process Source ---
			if (pg_s.root_level_page) {
				pageint p = _IMM(sors);
				auto crt = pg_s.getEntry(p);
				if (_IMM(crt) == ~_IMM0) return 0;
				stduint shift = VPN_SHIFTS[p.crt_level];
				stduint mask = (1ULL << shift) - 1;
				stduint offset = _IMM(sors) & mask;
				phy_s = crt->getAddress(p.crt_level) + offset;
				MIN(unit, (1ULL << shift) - offset);
			}
			else {
				phy_s = _IMM(sors);
			}

			for0(i, unit) {
				char ch = cast<char*>(phy_s)[i];
				if (!ch) {
					cast<char*>(phy_d)[i] = nil; // ASCIZ zero-termination
					return ret += i;
				}
				cast<char*>(phy_d)[i] = ch;
			}
			dest += unit, sors += unit;
			length -= unit;
			ret += unit;
		}

		// --- Handle Trailing Null ---
		if (length) {
			stduint phy_d;
			if (pg_d.root_level_page) {
				pageint p = _IMM(dest);
				auto crt = pg_d.getEntry(p);
				if (_IMM(crt) == ~_IMM0) return ret;
				stduint shift = VPN_SHIFTS[p.crt_level];
				phy_d = crt->getAddress(p.crt_level) + (_IMM(dest) & ((1ULL << shift) - 1));
			}
			else {
				phy_d = _IMM(dest);
			}
			treat<char>(phy_d) = nil;
		}
		return ret;
	}


}

#if defined(_ARC_x86) || defined(_ARC_x64)

namespace {
	constexpr uint64 PAGING_PAT_VALUE = 0x0007040100070406ULL;
	constexpr uint8 X86_MEMORY_UC = 0x00;
	constexpr uint8 X86_MEMORY_WC = 0x01;
	constexpr uint8 X86_MEMORY_WT = 0x04;
	constexpr uint8 X86_MEMORY_WP = 0x05;
	constexpr uint8 X86_MEMORY_WB = 0x06;

	static stduint X86PhysicalAddressBits() {
		unsigned a{}, b{}, c{}, d{};
		_IO_CPUID(0x80000000, 0, &a, &b, &c, &d);
		if (a < 0x80000008u) return 36;
		_IO_CPUID(0x80000008, 0, &a, &b, &c, &d);
		const stduint bits = a & 0xFFu;
		return bits >= 32 && bits <= 64 ? bits : 36;
	}

	static bool X86PATSupported() {
		unsigned a{}, b{}, c{}, d{};
		_IO_CPUID(1, 0, &a, &b, &c, &d);
		return !!(d & (1u << 16));
	}

	static bool X86PSE36Supported() {
		unsigned a{}, b{}, c{}, d{};
		_IO_CPUID(1, 0, &a, &b, &c, &d);
		return !!(d & (1u << 17));
	}

	static bool DecodeX86MemoryType(uint8 memory_type, stduint& type) {
		switch (memory_type) {
		case X86_MEMORY_UC: type = PGTYPE_uncacheable; return true;
		case X86_MEMORY_WC: type = PGTYPE_writecombine; return true;
		case X86_MEMORY_WT: type = PGTYPE_writethrough; return true;
		case X86_MEMORY_WB: type = PGTYPE_writeback; return true;
		case X86_MEMORY_WP: return false;
		default: return false;
		}
	}

	static bool MergeX86MTRRType(uint8& current, uint8 incoming) {
		if (current == incoming) return true;
		if (current == X86_MEMORY_UC || incoming == X86_MEMORY_UC) {
			current = X86_MEMORY_UC;
			return true;
		}
		if ((current == X86_MEMORY_WT && incoming == X86_MEMORY_WB) ||
			(current == X86_MEMORY_WB && incoming == X86_MEMORY_WT)) {
			current = X86_MEMORY_WT;
			return true;
		}
		return false;
	}

	static uint8 ReadX86FixedMTRRType(uint64 physical_address) {
		x86MSR msr;
		stduint index;
		if (physical_address < 0x80000) {
			msr = x86MSR::MTRR_FIX64K_00000;
			index = physical_address >> 16;
		}
		else if (physical_address < 0xA0000) {
			msr = x86MSR::MTRR_FIX16K_80000;
			index = (physical_address - 0x80000) >> 14;
		}
		else if (physical_address < 0xC0000) {
			msr = x86MSR::MTRR_FIX16K_A0000;
			index = (physical_address - 0xA0000) >> 14;
		}
		else {
			const stduint slot = (physical_address - 0xC0000) >> 15;
			msr = static_cast<x86MSR>(uint32(x86MSR::MTRR_FIX4K_C0000) + slot);
			index = ((physical_address - 0xC0000) >> 12) & 7;
		}
		return uint8(getMSR(msr) >> (index * 8));
	}
}

bool uni::EnablePagingPAT() {
	if (!X86PATSupported()) return false;
	setMSR(x86MSR::PAT, PAGING_PAT_VALUE);
	return getMSR(x86MSR::PAT) == PAGING_PAT_VALUE;
}

bool uni::PagingPATEnabled() {
	return X86PATSupported() && getMSR(x86MSR::PAT) == PAGING_PAT_VALUE;
}

bool uni::PagingPhysicalAddressSupported(uint64 physical_address, stduint level) {
	stduint bits = X86PhysicalAddressBits();
	#if defined(_ARC_x86)
	if (!level) bits = 32;
	else {
		if (level != 1) return false;
		if (!X86PSE36Supported()) bits = 32;
		else if (bits > 36) bits = 36;
	}
	#elif defined(_ARC_x64)
	if (level > 2) return false;
	if (bits > 52) bits = 52;
	#endif
	return bits == 64 || physical_address < (uint64(1) << bits);
}

bool uni::ReadX86MTRRType(uint64 physical_address, stduint& type) {
	unsigned a{}, b{}, c{}, d{};
	_IO_CPUID(1, 0, &a, &b, &c, &d);
	if (!(d & (1u << 12))) return false;
	const uint64 capability = getMSR(x86MSR::MTRRCAP);
	const uint64 default_type = getMSR(x86MSR::MTRR_DEF_TYPE);
	if (!(default_type & (uint64(1) << 11))) {
		type = PGTYPE_uncacheable;
		return true;
	}
	if (physical_address < 0x100000 && (capability & (uint64(1) << 8)) &&
		(default_type & (uint64(1) << 10)))
		return DecodeX86MemoryType(ReadX86FixedMTRRType(physical_address), type);

	const stduint physical_bits = X86PhysicalAddressBits();
	if (physical_bits < 64 && physical_address >= (uint64(1) << physical_bits)) return false;
	const uint64 physical_mask = physical_bits == 64 ? ~uint64(0) :
		((uint64(1) << physical_bits) - 1);
	const uint64 address_mask = physical_mask & ~uint64(0xFFF);
	const stduint variable_count = capability & 0xFF;
	bool matched = false;
	uint8 memory_type = 0;
	for (stduint index = 0; index < variable_count; index++) {
		const x86MSR base_msr = static_cast<x86MSR>(
			uint32(x86MSR::MTRR_PHYSBASE0) + uint32(index * 2));
		const x86MSR mask_msr = static_cast<x86MSR>(
			uint32(x86MSR::MTRR_PHYSMASK0) + uint32(index * 2));
		const uint64 mask = getMSR(mask_msr);
		if (!(mask & (uint64(1) << 11))) continue;
		const uint64 base = getMSR(base_msr);
		if ((physical_address & (mask & address_mask)) !=
			(base & (mask & address_mask))) continue;
		const uint8 incoming = uint8(base & 0xFF);
		if (!matched) {
			memory_type = incoming;
			matched = true;
		}
		else if (!MergeX86MTRRType(memory_type, incoming)) return false;
	}
	if (!matched) memory_type = uint8(default_type & 0xFF);
	return DecodeX86MemoryType(memory_type, type);
}

#endif

#if defined(_ARC_x86)

bool uni::Paging::isSupportedSize(stduint exponent) {
	unsigned a{}, b{}, c{}, d{};
	switch (exponent) {
	case PAGESIZE_4KB: return true;
	case PAGESIZE_4MB:
		_IO_CPUID(1, 0, &a, &b, &c, &d);
		return (d & (1u << 3)) != 0;
	case PAGESIZE_ANY: return false;
	default: return false;
	}
}

#elif defined(_ARC_x64)

static bool paging_nx_enabled = false;

bool uni::EnablePagingNX() {
	unsigned a{}, b{}, c{}, d{};
	_IO_CPUID(0x80000000, 0, &a, &b, &c, &d);
	if (a < 0x80000001u) return false;
	_IO_CPUID(0x80000001, 0, &a, &b, &c, &d);
	if (!(d & (1u << 20))) return false;

	constexpr uint64 EFER_NXE = uint64(1) << 11;
	setMSR(x86MSR::EFER, getMSR(x86MSR::EFER) | EFER_NXE);
	paging_nx_enabled = !!(getMSR(x86MSR::EFER) & EFER_NXE);
	return paging_nx_enabled;
}

bool uni::PagingNXEnabled() {
	return paging_nx_enabled;
}

bool uni::Paging::isSupportedSize(stduint exponent) {
	unsigned a{}, b{}, c{}, d{};
	switch (exponent) {
	case PAGESIZE_4KB: return true;
	case PAGESIZE_2MB:
		_IO_CPUID(1, 0, &a, &b, &c, &d);
		return (d & (1u << 3)) != 0;
	case PAGESIZE_1GB: {
		_IO_CPUID(0x80000000, 0, &a, &b, &c, &d);
		if (a < 0x80000001u) return false;
		_IO_CPUID(0x80000001, 0, &a, &b, &c, &d);
		return (d & (1u << 26)) != 0;
	}
	case PAGESIZE_ANY: return false;
	default: return false;
	}
}

#elif defined(_ARC_ARM_32)
#include "../../../../inc/c/proctrl/ARM.h"

bool uni::PagingARM32MemoryTypesEnabled() {
	stduint sctlr;
	stduint ttbcr;
	_ASM volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
	_ASM volatile("mrc p15, 0, %0, c2, c0, 2" : "=r"(ttbcr));
	return (sctlr & (_IMM1 << 23)) &&
		!(sctlr & (_IMM1 << 28)) && !(ttbcr & (_IMM1 << 31));
}

bool uni::Paging::isSupportedSize(stduint exponent) {
	return exponent == PAGESIZE_4KB || exponent == PAGESIZE_1MB;
}

#elif defined(_ARC_ARM_64)
#include "../../../../inc/c/proctrl/ARM.h"

bool uni::EnablePagingMAIR() {
	constexpr uint64 mair = 0x0000000044BB00FFULL;
	_ASM volatile("msr mair_el1, %0" : : "r"(mair) : "memory");
	_ASM volatile("isb" : : : "memory");
	uint64 current;
	_ASM volatile("mrs %0, mair_el1" : "=r"(current));
	return current == mair;
}

bool uni::PagingMAIREnabled() {
	constexpr uint64 mair = 0x0000000044BB00FFULL;
	uint64 current;
	_ASM volatile("mrs %0, mair_el1" : "=r"(current));
	return current == mair;
}

bool uni::Paging::isSupportedSize(stduint exponent) {
	return exponent == PAGESIZE_4KB ||
		exponent == PAGESIZE_2MB || exponent == PAGESIZE_1GB;
}

#elif defined(_ARC_RISCV_32)
#include "../../../../inc/c/proctrl/RISCV/riscv.h"

void uni::SetPagingSvpbmtSupported(bool) {}

bool uni::PagingSvpbmtSupported() {
	return false;
}

bool uni::Paging::isSupportedSize(stduint exponent) {
	return exponent == PAGESIZE_4KB || exponent == PAGESIZE_4MB;
}

#elif defined(_ARC_RISCV_64)
#include "../../../../inc/c/proctrl/RISCV/riscv.h"

static bool paging_svpbmt_supported = false;

void uni::SetPagingSvpbmtSupported(bool supported) {
	paging_svpbmt_supported = supported;
}

bool uni::PagingSvpbmtSupported() {
	return paging_svpbmt_supported;
}

bool uni::Paging::isSupportedSize(stduint exponent) {
	return exponent == PAGESIZE_4KB ||
		exponent == PAGESIZE_2MB || exponent == PAGESIZE_1GB;
}
#endif


namespace uni {

	static bool TransactionIncrement(stduint& value) {
		if (value == ~_IMM0) return false;
		value++;
		return true;
	}

	static bool EncodeTransactionLeaf(PageEntry& result, uint64 physical_address,
		stduint level, stduint properties) {
		result.Clear();
		return result.SetupAsLeaf(physical_address, level, properties) &&
			uint64(result.getAddress(level)) == physical_address &&
			PagePropertiesCompatible(result, level, properties);
	}

	static bool RecordTransactionWrite(PageMapTransaction& transaction,
		PageEntry* target, const PageEntry& value, stduint virtual_address,
		bool published) {
		if (!published) {
			if (transaction.build) target->raw = value.raw;
			return true;
		}
		if (!transaction.build) return TransactionIncrement(transaction.write_count);
		if (transaction.direct_write) {
			if (transaction.write_index >= transaction.write_count) return false;
			transaction.write_index++;
			#if defined(_ARC_ARM_32) || defined(_ARC_ARM_64)
			if (target->isPresent()) {
				target->raw = 0;
				RefreshVirtualAddress(virtual_address);
			}
			#endif
			target->raw = value.raw;
			RefreshVirtualAddress(virtual_address);
			return true;
		}
		if (transaction.write_index >= transaction.write_count) return false;
		PageMapWrite& write = transaction.writes[transaction.write_index++];
		write.target = target;
		write.expected_raw = target->raw;
		write.value.raw = value.raw;
		write.virtual_address = virtual_address;
		return true;
	}

	static bool CountTransactionEntry(PageMapTransaction& transaction,
		const PageEntry* entry, stduint level, stduint virtual_address,
		uint64 physical_address, stduint length, bool published);

	static bool CountTransactionChildren(PageMapTransaction& transaction,
		const PageEntry* table, stduint level, stduint virtual_address,
		uint64 physical_address, stduint length, bool published) {
		while (length) {
			const stduint unit = _IMM1 << VPN_SHIFTS[level];
			const stduint offset = virtual_address & (unit - 1);
			const stduint step = minof(unit - offset, length);
			const PageEntry* child = table ? &table[getVPN(virtual_address, level)] : nullptr;
			if (!CountTransactionEntry(transaction, child, level, virtual_address,
				physical_address, step, published)) return false;
			length -= step;
			if (length) {
				virtual_address += step;
				physical_address += step;
			}
		}
		return true;
	}

	static bool CountTransactionSplit(PageMapTransaction& transaction,
		const PageEntry& leaf, stduint level, stduint virtual_address,
		uint64 physical_address, stduint length, bool published) {
		if (!level || !PAGE_LEVEL_CAN_LEAF[level] || !PAGE_LEVEL_CAN_LEAF[level - 1] ||
			!TransactionIncrement(transaction.table_count)) return false;

		stduint existing_properties;
		if (!leaf.DecodeLeafProperties(level, existing_properties)) return false;
		const uint64 parent_unit = uint64(1) << VPN_SHIFTS[level];
		const uint64 child_unit = uint64(1) << VPN_SHIFTS[level - 1];
		const uint64 physical_base = leaf.getAddress(level);
		if ((physical_base & (parent_unit - 1)) ||
			parent_unit / child_unit != PAGE_ENTRY_COUNTS[level - 1]) return false;

		for (stduint index = 0; index < PAGE_ENTRY_COUNTS[level - 1]; index++) {
			const uint64 child_physical = physical_base + uint64(index) * child_unit;
			PageEntry child;
			if (child_physical < physical_base ||
				!EncodeTransactionLeaf(child, child_physical, level - 1,
					existing_properties)) return false;
		}

		while (length) {
			const stduint child_level = level - 1;
			const stduint child_size = _IMM1 << VPN_SHIFTS[child_level];
			const stduint offset = virtual_address & (child_size - 1);
			const stduint step = minof(child_size - offset, length);
			const stduint index = getVPN(virtual_address, child_level);
			PageEntry child;
			if (!EncodeTransactionLeaf(child,
				physical_base + uint64(index) * child_unit,
				child_level, existing_properties) ||
				!CountTransactionEntry(transaction, &child, child_level,
					virtual_address, physical_address, step, false)) return false;
			length -= step;
			if (length) {
				virtual_address += step;
				physical_address += step;
			}
		}

		PageEntry table_entry;
		table_entry.Clear();
		return RecordTransactionWrite(transaction, const_cast<PageEntry*>(&leaf),
			table_entry, virtual_address, published);
	}

	static bool CountTransactionEmptyTable(PageMapTransaction& transaction,
		const PageEntry* entry, stduint level, stduint virtual_address,
		uint64 physical_address, stduint length, bool published) {
		if (!level || !TransactionIncrement(transaction.table_count) ||
			!CountTransactionChildren(transaction, nullptr, level - 1,
				virtual_address, physical_address, length, false)) return false;
		PageEntry table_entry;
		table_entry.Clear();
		return RecordTransactionWrite(transaction, const_cast<PageEntry*>(entry),
			table_entry, virtual_address, published);
	}

	static bool CountTransactionEntry(PageMapTransaction& transaction,
		const PageEntry* entry, stduint level, stduint virtual_address,
		uint64 physical_address, stduint length, bool published) {
		const bool present = entry && entry->isPresent();
		const stduint unit = _IMM1 << VPN_SHIFTS[level];
		const stduint offset = virtual_address & (unit - 1);
		const bool full_leaf = !offset && length == unit;

		if (!present) {
			transaction.saw_not_present = true;
			if (!(transaction.properties & PGPROP_present)) return true;
			if (full_leaf && level <= transaction.maximum_level &&
				PAGE_LEVEL_CAN_LEAF[level] && Paging::isSupportedSize(VPN_SHIFTS[level])) {
				PageEntry replacement;
				if (EncodeTransactionLeaf(replacement, physical_address, level,
					transaction.properties))
					return RecordTransactionWrite(transaction, const_cast<PageEntry*>(entry),
						replacement, virtual_address, published);
			}
			if (!level) return false;
			return CountTransactionEmptyTable(transaction, entry, level,
				virtual_address, physical_address, length, published);
		}

		if (entry->isTable(level)) {
			if (!level) return false;
			if (!transaction.any_size && published && level <= transaction.maximum_level &&
				!transaction.finer_structure) {
				transaction.finer_structure = true;
				transaction.finer_address = virtual_address;
				transaction.finer_level = level - 1;
			}
			const PageEntry* child_table = (const PageEntry*)entry->getAddress(level);
			if (!child_table) return false;
			return CountTransactionChildren(transaction, child_table, level - 1,
				virtual_address, physical_address, length, true);
		}

		if (!entry->isLeaf(level)) return false;
		transaction.saw_present = true;
		if (transaction.properties & PGPROP_weak) return true;

		const bool properties_compatible =
			PagePropertiesCompatible(*entry, level, transaction.properties);
		const bool mapping_compatible = (transaction.properties & PGPROP_present) &&
			uint64(entry->getAddress(level)) + offset == physical_address &&
			properties_compatible;
		if (mapping_compatible) return true;

		if ((transaction.properties & PGPROP_present) && !properties_compatible &&
			!transaction.property_change) {
			transaction.property_change = true;
			transaction.property_address = virtual_address;
			transaction.property_level = level;
		}

		stduint replacement_properties = transaction.properties;
		if ((replacement_properties & PGPROP_present) &&
			(replacement_properties & PGPROP_MASK_TYPE) == PGTYPE_default) {
			stduint existing_properties;
			if (!entry->DecodeLeafProperties(level, existing_properties)) return false;
			replacement_properties |= existing_properties & PGPROP_MASK_TYPE;
		}

		if (full_leaf && level <= transaction.maximum_level) {
			PageEntry replacement;
			replacement.Clear();
			if (!(replacement_properties & PGPROP_present) ||
				EncodeTransactionLeaf(replacement, physical_address, level,
					replacement_properties))
				return RecordTransactionWrite(transaction, const_cast<PageEntry*>(entry),
					replacement, virtual_address, published);
		}

		if (!level) return false;
		return CountTransactionSplit(transaction, *entry, level,
			virtual_address, physical_address, length, published);
	}

	static bool BuildTransactionEntry(PageMapTransaction& transaction,
		PageEntry* entry, stduint level, stduint virtual_address,
		uint64 physical_address, stduint length, bool published);

	static PageEntry* AcquireTransactionTable(PageMapTransaction& transaction) {
		if (transaction.table_index >= transaction.table_count) return nullptr;
		PageEntry* table = transaction.tables[transaction.table_index++];
		MemSet(table, 0, 0x1000);
		return table;
	}

	static bool BuildTransactionChildren(PageMapTransaction& transaction,
		PageEntry* table, stduint level, stduint virtual_address,
		uint64 physical_address, stduint length, bool published) {
		while (length) {
			const stduint unit = _IMM1 << VPN_SHIFTS[level];
			const stduint offset = virtual_address & (unit - 1);
			const stduint step = minof(unit - offset, length);
			if (!BuildTransactionEntry(transaction, &table[getVPN(virtual_address, level)],
				level, virtual_address, physical_address, step, published)) return false;
			length -= step;
			if (length) {
				virtual_address += step;
				physical_address += step;
			}
		}
		return true;
	}

	static bool BuildTransactionSplit(PageMapTransaction& transaction,
		PageEntry* leaf, stduint level, stduint virtual_address,
		uint64 physical_address, stduint length, bool published) {
		stduint existing_properties;
		if (!leaf->DecodeLeafProperties(level, existing_properties)) return false;
		const uint64 parent_unit = uint64(1) << VPN_SHIFTS[level];
		const uint64 child_unit = uint64(1) << VPN_SHIFTS[level - 1];
		const uint64 physical_base = leaf->getAddress(level);
		PageEntry* child_table = AcquireTransactionTable(transaction);
		if (!child_table || (physical_base & (parent_unit - 1)) ||
			parent_unit / child_unit != PAGE_ENTRY_COUNTS[level - 1]) return false;

		for (stduint index = 0; index < PAGE_ENTRY_COUNTS[level - 1]; index++) {
			const uint64 child_physical = physical_base + uint64(index) * child_unit;
			if (child_physical < physical_base ||
				!EncodeTransactionLeaf(child_table[index], child_physical,
					level - 1, existing_properties)) return false;
		}
		if (!BuildTransactionChildren(transaction, child_table, level - 1,
			virtual_address, physical_address, length, false)) return false;

		PageEntry table_entry;
		if (!table_entry.SetupAsTable(child_table, level) ||
			uint64(table_entry.getAddress(level)) != uint64(_IMM(child_table))) return false;
		return RecordTransactionWrite(transaction, leaf, table_entry,
			virtual_address, published);
	}

	static bool BuildTransactionEmptyTable(PageMapTransaction& transaction,
		PageEntry* entry, stduint level, stduint virtual_address,
		uint64 physical_address, stduint length, bool published) {
		PageEntry* child_table = AcquireTransactionTable(transaction);
		if (!child_table || !BuildTransactionChildren(transaction, child_table,
			level - 1, virtual_address, physical_address, length, false)) return false;
		PageEntry table_entry;
		if (!table_entry.SetupAsTable(child_table, level) ||
			uint64(table_entry.getAddress(level)) != uint64(_IMM(child_table))) return false;
		return RecordTransactionWrite(transaction, entry, table_entry,
			virtual_address, published);
	}

	static bool BuildTransactionEntry(PageMapTransaction& transaction,
		PageEntry* entry, stduint level, stduint virtual_address,
		uint64 physical_address, stduint length, bool published) {
		const bool present = entry->isPresent();
		const stduint unit = _IMM1 << VPN_SHIFTS[level];
		const stduint offset = virtual_address & (unit - 1);
		const bool full_leaf = !offset && length == unit;

		if (!present) {
			if (!(transaction.properties & PGPROP_present)) return true;
			if (full_leaf && level <= transaction.maximum_level &&
				PAGE_LEVEL_CAN_LEAF[level] && Paging::isSupportedSize(VPN_SHIFTS[level])) {
				PageEntry replacement;
				if (EncodeTransactionLeaf(replacement, physical_address, level,
					transaction.properties))
					return RecordTransactionWrite(transaction, entry, replacement,
						virtual_address, published);
			}
			if (!level) return false;
			return BuildTransactionEmptyTable(transaction, entry, level,
				virtual_address, physical_address, length, published);
		}

		if (entry->isTable(level)) {
			if (!level) return false;
			PageEntry* child_table = (PageEntry*)entry->getAddress(level);
			if (!child_table) return false;
			return BuildTransactionChildren(transaction, child_table, level - 1,
				virtual_address, physical_address, length, true);
		}

		if (!entry->isLeaf(level)) return false;
		if (transaction.properties & PGPROP_weak) return true;
		const bool properties_compatible =
			PagePropertiesCompatible(*entry, level, transaction.properties);
		const bool mapping_compatible = (transaction.properties & PGPROP_present) &&
			uint64(entry->getAddress(level)) + offset == physical_address &&
			properties_compatible;
		if (mapping_compatible) return true;

		stduint replacement_properties = transaction.properties;
		if ((replacement_properties & PGPROP_present) &&
			(replacement_properties & PGPROP_MASK_TYPE) == PGTYPE_default) {
			stduint existing_properties;
			if (!entry->DecodeLeafProperties(level, existing_properties)) return false;
			replacement_properties |= existing_properties & PGPROP_MASK_TYPE;
		}

		if (full_leaf && level <= transaction.maximum_level) {
			PageEntry replacement;
			replacement.Clear();
			if (!(replacement_properties & PGPROP_present) ||
				EncodeTransactionLeaf(replacement, physical_address, level,
					replacement_properties))
				return RecordTransactionWrite(transaction, entry, replacement,
					virtual_address, published);
		}

		if (!level) return false;
		return BuildTransactionSplit(transaction, entry, level,
			virtual_address, physical_address, length, published);
	}

	static bool CountTransactionRange(PageMapTransaction& transaction,
		PageEntry* root, stduint virtual_address, uint64 physical_address,
		stduint length) {
		return CountTransactionChildren(transaction, root, PAGE_LEVELS - 1,
			virtual_address, physical_address, length, true);
	}

	static bool BuildTransactionRange(PageMapTransaction& transaction,
		PageEntry* root, stduint virtual_address, uint64 physical_address,
		stduint length) {
		return BuildTransactionChildren(transaction, root, PAGE_LEVELS - 1,
			virtual_address, physical_address, length, true);
	}

	static void ReleaseTransactionStorage(PageMapTransaction& transaction,
		stduint allocated_tables, bool release_tables) {
		if (!uni_default_allocator) return;
		if (release_tables && transaction.tables) {
			for (stduint index = 0; index < allocated_tables; index++)
				uni_default_allocator->deallocate(transaction.tables[index], 0x1000);
		}
		if (transaction.writes)
			uni_default_allocator->deallocate(transaction.writes,
				transaction.write_count * sizeof(PageMapWrite));
		if (transaction.tables)
			uni_default_allocator->deallocate(transaction.tables,
				transaction.table_count * sizeof(PageEntry*));
		transaction.writes = nullptr;
		transaction.tables = nullptr;
	}

	static bool ExecutePageMapTransaction(PageMapTransaction& transaction,
		PageEntry* root, stduint virtual_address, uint64 physical_address,
		stduint length) {
		if (!root || !CountTransactionRange(transaction, root, virtual_address,
			physical_address, length)) return false;
		if (!transaction.write_count) return !transaction.table_count;
		if (!uni_default_allocator && !transaction.table_count) {
			transaction.build = true;
			transaction.direct_write = true;
			return BuildTransactionRange(transaction, root, virtual_address,
				physical_address, length) &&
				transaction.write_index == transaction.write_count;
		}
		if (!uni_default_allocator ||
			transaction.table_count > ~_IMM0 / sizeof(PageEntry*) ||
			transaction.write_count > ~_IMM0 / sizeof(PageMapWrite)) return false;

		if (transaction.table_count) {
			transaction.tables = (PageEntry**)uni_default_allocator->allocate(
				transaction.table_count * sizeof(PageEntry*));
			if (!transaction.tables) return false;
		}
		transaction.writes = (PageMapWrite*)uni_default_allocator->allocate(
			transaction.write_count * sizeof(PageMapWrite));
		if (!transaction.writes) {
			ReleaseTransactionStorage(transaction, 0, false);
			return false;
		}

		stduint allocated_tables = 0;
		for (; allocated_tables < transaction.table_count; allocated_tables++) {
			void* table = uni_default_allocator->allocate(0x1000, PAGESIZE_4KB);
			if (!table) {
				ReleaseTransactionStorage(transaction, allocated_tables, true);
				return false;
			}
			transaction.tables[allocated_tables] = (PageEntry*)table;
			MemSet(table, 0, 0x1000);
		}

		transaction.build = true;
		if (!BuildTransactionRange(transaction, root, virtual_address,
			physical_address, length) ||
			transaction.table_index != transaction.table_count ||
			transaction.write_index != transaction.write_count) {
			ReleaseTransactionStorage(transaction, allocated_tables, true);
			return false;
		}
		for (stduint index = 0; index < transaction.write_count; index++) {
			const stduint current = transaction.writes[index].target->raw;
			const stduint changed = current ^ transaction.writes[index].expected_raw;
			#if defined(_ARC_x86) || defined(_ARC_x64)
			const stduint hardware_changed = (_IMM1 << 5) | (_IMM1 << 6);
			#else
			const stduint hardware_changed = 0;
			#endif
			if (changed & ~hardware_changed) {
				ReleaseTransactionStorage(transaction, allocated_tables, true);
				return false;
			}
		}
		for (stduint index = 0; index < transaction.write_count; index++) {
			PageMapWrite& write = transaction.writes[index];
			#if defined(_ARC_ARM_32) || defined(_ARC_ARM_64)
			PageEntry existing;
			existing.raw = write.expected_raw;
			if (existing.isPresent()) {
				write.target->raw = 0;
				RefreshVirtualAddress(write.virtual_address);
			}
			#endif
			write.target->raw = write.value.raw;
			RefreshVirtualAddress(write.virtual_address);
		}

		ReleaseTransactionStorage(transaction, allocated_tables, false);
		return true;
	}
}

