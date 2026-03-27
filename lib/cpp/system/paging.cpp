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

#include "../../../inc/c/system/paging.h"
#include "../../../inc/c/ustring.h"

// a32 (unchk)
// r64 (unchk)
// a64 (unchk)


#if defined(_ARC_x86) || defined(_ARC_RISCV_32)
// x86 Lev2: 00000000~FFFFFFFF
// r32 Lev2: 00000000~FFFFFFFF (unchk)
//     [v]4KB [v]4MB
// [x86] 4MB
// - set CR4.PSE
// - set PDE.PAT of l1p_index
// [RISCV] 
// - if R = W = X = 0 then current table is the table of table but a page
constexpr stduint PAGE_LEVELS = 2;
constexpr stduint VPN_SHIFTS[] = {12, 22};
constexpr stduint VPN_MASKS[] = { 0x3FF, 0x3FF };

#elif defined(_ARC_x64) || defined(_ARC_ARM_64)
// x64 Lev4: 000000000000~FFFFFFFFFFFF
//     [v]4KB [v]2MB [v]1GB ---
// x64 2MB/1GB
// - enable PAE
// - set PDE.PAT of l1p_index(2M) or l2p_index(1G)
constexpr stduint PAGE_LEVELS = 4;
constexpr stduint VPN_SHIFTS[] = {12, 21, 30, 39};
constexpr stduint VPN_MASKS[] = { 0x1FF, 0x1FF, 0x1FF, 0x1FF };

#elif defined(_ARC_RISCV_64) 

constexpr stduint PAGE_LEVELS = 3;
constexpr stduint VPN_SHIFTS[] = {12, 21, 30};
constexpr stduint VPN_MASKS[]  = {0x1FF, 0x1FF, 0x1FF};
#endif

namespace uni {
	#if !defined(_ARC_ARM_32)
	inline stduint getVPN(stduint address, stduint level) {
		return (address >> VPN_SHIFTS[level]) & VPN_MASKS[level];
	}
	#endif
}

namespace uni {

	#if defined(_ARC_ARM_32)

	Paging::~Paging() {
		if (!root_level_page || !uni_default_allocator) return;
		for (int i = 0; i < 4096; i++) {
			PageEntry& pde = root_level_page[i];
			if (pde.isPresent() && !pde.isHuge()) {
				void* pt = (void*)pde.getAddress();
				uni_default_allocator->deallocate(pt, 0x1000);
			}
		}
		uni_default_allocator->deallocate(root_level_page, 0x4000);
		root_level_page = nullptr;
	}

	auto Paging::refEntry(pageint p) -> PageEntry* {
		return _TEMP getEntry((stduint)p);
	}

	auto Paging::getEntry(stduint address) const -> PageEntry* {
		pageint p = address;
		auto pe = (PageEntry*)root_level_page + p.l1p_index;
		if (!pe->isPresent()) return (PageEntry*)~_IMM0;
		if (pe->isHuge()) return pe;
		pe = (PageEntry*)(pe->getAddress()) + p.l0p_index;
		return pe;
	}

	auto Paging::getPageSizeShift(stduint address) const -> stduint {
		pageint p = address;
		auto pe = (PageEntry*)root_level_page + p.l1p_index;
		if (!pe->isPresent()) return 0;
		if (pe->isHuge()) return 20; // 1MB Section

		pe = (PageEntry*)(pe->getAddress()) + p.l0p_index;
		if (!pe->isPresent()) return 0;
		return 12; // 4KB Page
	}

	auto Paging::PageMap(stduint laddr, stduint paddr, stduint pgsize, stduint pgporp) -> bool {
		if (pgsize == 12 || pgsize == 20); else return false;
		pageint p = laddr;
		auto pe = (PageEntry*)root_level_page + p.l1p_index;

		if (pgsize == 20) {
			if (pgporp & PGPROP_present) pe->SetupAsLeaf(paddr, true, pgporp);
			else pe->Clear();
			return true;
		}

		if (!pe->isPresent()) {
			if (!(pgporp & PGPROP_present)) return true;
			void* new_pg = uni_default_allocator ?
				uni_default_allocator->allocate(0x1000, 0x1000) : nullptr;
			if (!new_pg) return false;
			MemSet(new_pg, 0, 0x1000);
			pe->SetupAsTable(new_pg);
		}

		pe = (PageEntry*)(pe->getAddress()) + p.l0p_index;
		if (pgporp & PGPROP_present) pe->SetupAsLeaf(paddr, false, pgporp);
		else pe->Clear();
		return true;
	}

	#else

	static void FreePageTable(PageEntry* table, int level) {
		if (!table) return;
		if (level > 0) {
			stduint max_entries = VPN_MASKS[level] + 1;
			for (stduint i = 0; i < max_entries; i++) {
				PageEntry& pe = table[i];
				if (pe.isPresent() && !pe.isHuge()) {
					PageEntry* next_table = (PageEntry*)pe.getAddress();
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

	// static byte getLevel(stduint lnaddr) {}

	auto Paging::refEntry(pageint p) -> PageEntry* {
		// needs pageint level
		if (p.crt_level >= PAGE_LEVELS) return (PageEntry*)~_IMM0;

		PageEntry* table = root_level_page;
		PageEntry* pe = nullptr;
		stduint address = (stduint)p;

		for (int level = PAGE_LEVELS - 1; level >= (int)p.crt_level; --level) {
			pe = &table[getVPN(address, level)];
			if (!pe->isPresent()) return (PageEntry*)~_IMM0;
			if (level > p.crt_level && pe->isHuge()) return (PageEntry*)~_IMM0;
			table = (PageEntry*)pe->getAddress();
		}
		return pe;
	}

	auto Paging::getEntry(stduint address) const -> PageEntry* {
		if (sizeof(stduint) == 8 && address >= (_IMM1 << (12 + PAGE_LEVELS * VPN_SHIFTS[0])))
			return (PageEntry*)~_IMM0;
		PageEntry* table = root_level_page;
		PageEntry* pe = nullptr;
		for (int level = PAGE_LEVELS - 1; level >= 0; --level) {
			pe = &table[getVPN(address, level)];
			if (!pe->isPresent()) return (PageEntry*)~_IMM0;
			if (pe->isHuge() || level == 0) return pe;
			table = (PageEntry*)pe->getAddress();
		}
		return pe;
	}

	auto Paging::getPageSizeShift(stduint address) const -> stduint {
		PageEntry* table = root_level_page;
		for (int level = PAGE_LEVELS - 1; level >= 0; --level) {
			PageEntry* pe = &table[getVPN(address, level)];
			if (!pe->isPresent()) return 0;
			if (pe->isHuge() || level == 0) return VPN_SHIFTS[level];
			table = (PageEntry*)pe->getAddress();
		}
		return 0;
	}

	auto Paging::PageMap(stduint laddr, stduint paddr, stduint pgsize, stduint pgporp) -> bool {
		int target_level = -1;
		for (int i = 0; i < PAGE_LEVELS; i++) {
			if (VPN_SHIFTS[i] == pgsize) { target_level = i; break; }
		}
		if (target_level == -1) return false;

		PageEntry* table = root_level_page;
		PageEntry* pe = nullptr;

		for (int level = PAGE_LEVELS - 1; level >= target_level; --level) {
			pe = &table[getVPN(laddr, level)];
			if (level == target_level) break;

			if (!pe->isPresent()) {
				if (!(pgporp & PGPROP_present)) return true;
				void* new_pg = uni_default_allocator ?
					uni_default_allocator->allocate(0x1000, 0x1000) : nullptr;
				if (!new_pg) return false;
				MemSet(new_pg, 0, 0x1000);
				pe->SetupAsTable(new_pg);
			}
			table = (PageEntry*)pe->getAddress();
		}

		if (pgporp & PGPROP_present) {
			pe->SetupAsLeaf(paddr, target_level > 0, pgporp);
		}
		else {
			pe->Clear();
		}
		return true;
	}
	#endif

	void* Paging::operator[](stduint address) const {
		auto entry = getEntry(address);
		if (_IMM(entry) == ~_IMM0 || !entry->isPresent()) return (void*)~_IMM0;

		stduint shift = getPageSizeShift(address);
		stduint mask = (1ULL << shift) - 1;
		return (void*)(entry->getAddress() + (address & mask));
	}

	void Paging::Reset() {
		stduint root_size = 0x1000;
		#if defined(_ARC_ARM_32)
		root_size = 0x4000;
		#endif

		if (uni_default_allocator) {
			root_level_page = (uni::PageEntry*)uni_default_allocator->allocate(root_size, root_size);
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
		stduint alignmask = (_IMM1 << pgsize) - 1;
		ph_address &= ~alignmask;
		if (ln_address + length - 1 < ln_address) return false;

		length += ln_address & alignmask;
		ln_address &= ~alignmask;
		const stduint unit = alignmask + 1;

		do {
			if (pgporp & PGPROP_weak) {
				auto entry = getEntry(ln_address);
				if (_IMM(entry) == ~_IMM0 || !entry->isPresent())
					PageMap(ln_address, ph_address, pgsize, pgporp);
			}
			else {
				PageMap(ln_address, ph_address, pgsize, pgporp);
			}
			ln_address += unit;
			ph_address += unit;
			length -= minof(unit, length);
		} while (length);

		return true;
	}

	auto Paging::Unmap(stduint ln_address, stduint length) -> bool {
		if (!length) return true;
		while (length > 0) {
			auto pe = getEntry(ln_address);
			if (_IMM(pe) != ~_IMM0 && pe->isPresent()) {
				stduint shift = getPageSizeShift(ln_address);
				stduint unit = _IMM1 << shift;
				stduint step = minof(unit - (ln_address & (unit - 1)), length);
				Map(ln_address, 0, step, (PageSizeShift)shift, 0);
				ln_address += step;
				length -= step;
			}
			else {
				stduint step = minof((_IMM1 << PAGESIZE_4KB) - (ln_address & ((_IMM1 << PAGESIZE_4KB) - 1)), length);
				ln_address += step;
				length -= step;
			}
		}
		return true;
	}



	stduint MemCopyP(void* dest, Paging& pg_d, const void* sors, Paging& pg_s, size_t length)
	{
		stduint ret = 0;

		while (length) {
			auto crtpage_d = pg_d.getEntry(_IMM(dest));
			auto crtpage_s = pg_s.getEntry(_IMM(sors));

			if (_IMM(crtpage_d) == ~_IMM0 || !crtpage_d->isPresent()) {
				plogerro(" at %s(dest %[32H], %[32H], sors %[32H], %[32H], length %[u])",
					__FUNCIDEN__, dest, pg_d.root_level_page, sors, pg_s.root_level_page, length
				);
				plogerro("MemCopyP: dest page is not present. rest %[u]B", length);
				return 0;
			}
			if (_IMM(crtpage_s) == ~_IMM0 || !crtpage_s->isPresent()) {
				plogerro("MemCopyP: sors page is not present");
				return 0;
			}

			stduint pgsize_d = pg_d.getPageSizeShift(_IMM(dest));
			stduint mask_d = (1ULL << pgsize_d) - 1;
			stduint pg_offset_d = _IMM(dest) & mask_d;
			stduint phy_d = crtpage_d->getAddress() + pg_offset_d;
			stduint page_bound_d = 1ULL << pgsize_d;

			stduint pgsize_s = pg_s.getPageSizeShift(_IMM(sors));
			stduint mask_s = (1ULL << pgsize_s) - 1;
			stduint pg_offset_s = _IMM(sors) & mask_s;
			stduint phy_s = crtpage_s->getAddress() + pg_offset_s;
			stduint page_bound_s = 1ULL << pgsize_s;

			stduint unit = length;
			MIN(unit, page_bound_d - pg_offset_d);
			MIN(unit, page_bound_s - pg_offset_s);

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
			auto crtpage_d = pg_d.getEntry(_IMM(dest));
			auto crtpage_s = pg_s.getEntry(_IMM(sors));

			if (_IMM(crtpage_d) == ~_IMM0 || !crtpage_d->isPresent()) return 0;
			if (_IMM(crtpage_s) == ~_IMM0 || !crtpage_s->isPresent()) return 0;

			stduint pgsize_d = pg_d.getPageSizeShift(_IMM(dest));
			stduint mask_d = (1ULL << pgsize_d) - 1;
			stduint pg_offset_d = _IMM(dest) & mask_d;
			stduint phy_d = crtpage_d->getAddress() + pg_offset_d;
			stduint page_bound_d = 1ULL << pgsize_d;

			stduint pgsize_s = pg_s.getPageSizeShift(_IMM(sors));
			stduint mask_s = (1ULL << pgsize_s) - 1;
			stduint pg_offset_s = _IMM(sors) & mask_s;
			stduint phy_s = crtpage_s->getAddress() + pg_offset_s;
			stduint page_bound_s = 1ULL << pgsize_s;

			stduint unit = length;
			MIN(unit, page_bound_d - pg_offset_d);
			MIN(unit, page_bound_s - pg_offset_s);

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

		if (1) {
			auto crtpage_d = pg_d.getEntry(_IMM(dest));
			if (_IMM(crtpage_d) == ~_IMM0 || !crtpage_d->isPresent()) return 0;

			stduint pgsize_d = pg_d.getPageSizeShift(_IMM(dest));
			stduint mask_d = (1ULL << pgsize_d) - 1;
			stduint pg_offset_d = _IMM(dest) & mask_d;
			stduint phy_d = crtpage_d->getAddress() + pg_offset_d;
			treat<char>(phy_d) = nil; // ASCIZ zero-termination
		}
		return ret;
	}


}
