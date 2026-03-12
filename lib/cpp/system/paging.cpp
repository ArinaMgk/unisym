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

namespace uni {

#if defined(_ARC_x86)

	auto Paging::refEntry(pageint p) -> PageEntry* {
		const unsigned X86_MAXLEV = 2;
		if (p.crt_level > X86_MAXLEV - 1) return (PageEntry*)~_IMM0;
		auto pe = root_level_page + p.l1p_index;
		const stduint indexes[X86_MAXLEV - 1] = {
			p.l0p_index
		};
		stduint lev = X86_MAXLEV - 1 - p.crt_level;
		for0r(i, lev) {
			const stduint index = indexes[i - 1];
			if (!pe->isPresent()) return (PageEntry*)~_IMM0;
			const stduint addr = _IMM(pe->address) << 12;
			pe = (PageEntry*)addr + index;
		}
		return pe;
	}

	auto Paging::getEntry(stduint address) const -> PageEntry* {
		if (sizeof(stduint) == 8 && address >= (_IMM1 << 48)) return (PageEntry*)~_IMM0;

		pageint p = address;

		// Level 1: PD
		auto pe = (PageEntry*)root_level_page + p.l1p_index;
		if (!pe->isPresent()) return (PageEntry*)~_IMM0; // unmap
		if (pe->huge_page) return pe; // 4MB

		// Level 0: PT
		pe = (PageEntry*)(_IMM(pe->address) << 12) + p.l0p_index;
		return pe;
	}

	void* Paging::operator[](stduint address) const {
		auto entry = getEntry(address);
		if (_IMM(entry) == ~_IMM0 || !entry->isPresent()) return (void*)~_IMM0;
		if (entry->huge_page) {
			stduint mask = (1ULL << (sizeof(stduint) == 4 ? 22 : 21)) - 1; 
			return (void*)((_IMM(entry->address) << 12) + (address & mask));
		}
		return (void*)((_IMM(entry->address) << 12) + (address & 0xFFF));
	}

	auto Paging::PageMap(stduint laddr, stduint paddr, stduint pgsize, stduint pgporp) -> bool {
		// pgporp is for multi-level page tables.
		if (pgsize == 12 || pgsize == 22); else return false;
		pageint p = laddr;
		
		stduint level = pgsize == 12 ? 0 : 1; 
		const unsigned X86_MAXLEV = 2;
		
		auto pe = root_level_page + p.l1p_index;
		const stduint indexes[X86_MAXLEV - 1] = {
			p.l0p_index
		};
		stduint lev = X86_MAXLEV - 1 - level;

		PageEntry* path[X86_MAXLEV];
		PageEntry* tables[X86_MAXLEV];
		tables[1] = (PageEntry*)root_level_page;
		path[1] = pe;

		for0(i, lev) {
			const stduint index = indexes[i];
			if (!pe->isPresent()) {
				if (!(pgporp & PGPROP_present)) return true; // Already unmapped
				stduint new_pg_addr = 0;
				if (uni_default_allocator) {
					new_pg_addr = _IMM(uni_default_allocator->allocate(0x1000, 0x1000));
				} else if (_physical_allocate) {
					new_pg_addr = _IMM(_physical_allocate(0x1000));
				} else return false;
				if (!new_pg_addr) return false;
				auto new_pg = (void*)new_pg_addr;
				MemSet(pe, 0, sizeof(PageEntry));
				MemSet(new_pg, 0, 0x1000);
				pe->address = _IMM(new_pg) >> 12;
				pe->present = _TEMP 1;
				pe->writable = true; 
				pe->user_access = true; 
				pe->global = nil; 
			}
			const stduint addr = _IMM(pe->address) << 12;
			tables[0 - i] = (PageEntry*)addr;
			pe = tables[0 - i] + index;
			path[0 - i] = pe;
		}

		if (pgporp & PGPROP_present) {
			// proc huge page
			if (level) {
				pe->huge_page = true;
			}
			pe->address = _IMM(paddr) >> 12;
			pe->present = _TEMP 1;
			pe->writable = !!(pgporp & PGPROP_writable);
			pe->user_access = !!(pgporp & PGPROP_user_access);
			pe->global = !!(pgporp & PGPROP_global);
		} else {
			pe->present = 0;
			pe->address = 0;
			for (stduint i = level; i < X86_MAXLEV - 1; i++) {
				bool empty = true;
				PageEntry* table = tables[i];
				for (int j = 0; j < 1024; j++) {
					if (table[j].present) {
						empty = false;
						break;
					}
				}
				if (empty) {
					if (uni_default_allocator) uni_default_allocator->deallocate(table, 0x1000); 
					path[i + 1]->present = 0;
					path[i + 1]->address = 0;
				} else {
					break;
				}
			}
		}
		return true;
	}
	
	auto Paging::Map(stduint ln_address, stduint ph_address, stduint length, stduint pgsize, stduint pgporp) -> bool {
		if (pgsize == 12 || pgsize == 22); else return false;
		if (!length) return true;
		stduint alignmask = (_IMM1 << pgsize) - 1;
		ph_address &= ~alignmask;
		if (ln_address >= ln_address + length) return false;
		length += ln_address & alignmask;
		ln_address &= ~alignmask;
		const stduint unit = alignmask + 1;
		if (pgporp & PGPROP_weak) do {
			auto entry = getEntry(ln_address);
			if (_IMM(entry) == ~_IMM0 || !entry->isPresent())
				PageMap(ln_address, ph_address, pgsize, pgporp);
			ln_address += unit, ph_address += unit;
			length -= minof(unit, length);
		} while (length);
		else do {
			PageMap(ln_address, ph_address, pgsize, pgporp);
			ln_address += unit, ph_address += unit;
			length -= minof(unit, length);
		} while (length);
		return true;
	}

	auto Paging::Unmap(stduint ln_address, stduint ph_address, stduint length, stduint pgsize) -> bool {
		return Map(ln_address, ph_address, length, pgsize, 0);
	}

	void Paging::Reset() {
		if (uni_default_allocator) {
			root_level_page = (uni::PageEntry*)uni_default_allocator->allocate(0x1000, 0x1000);
		} else if (_physical_allocate) {
			root_level_page = (uni::PageEntry*)_physical_allocate(0x1000);
		}
		if (root_level_page) MemSet(root_level_page, 0, 0x1000);
	}

#elif defined(_ARC_x64)// IA32e

	// static byte getLevel(stduint lnaddr) {}
	
	auto Paging::refEntry(pageint p) -> PageEntry* {
		const unsigned X64_MAXLEV = 4;
		if (p.crt_level > X64_MAXLEV - 1) return (PageEntry*)~_IMM0;
		auto pe = root_level_page + p.l3p_index;
		const stduint indexes[X64_MAXLEV - 1] = {
			p.l0p_index, p.l1p_index, p.l2p_index
		};
		stduint lev = X64_MAXLEV - 1 - p.crt_level;
		for0r(i, lev) {
			const stduint index = indexes[i - 1];
			if (!pe->isPresent()) return (PageEntry*)~_IMM0;
			const stduint addr = _IMM(pe->address) << 12;
			pe = (PageEntry*)addr + index;
		}
		return pe;
	}
	
	auto Paging::getEntry(stduint address) const -> PageEntry* {
		if (address >= (_IMM1 << 48)) return (PageEntry*)~_IMM0;

		pageint p = address;
		const unsigned X64_MAXLEV = 4;

		// Level 4: PML4
		auto pe = (PageEntry*)root_level_page + p.l3p_index;
		if (!pe->isPresent()) return (PageEntry*)~_IMM0; // unmap

		// Level 3: PDP
		pe = (PageEntry*)(_IMM(pe->address) << 12) + p.l2p_index;
		if (!pe->isPresent()) return (PageEntry*)~_IMM0;
		if (pe->huge_page) return pe; // 1GB

		// Level 2: PD
		pe = (PageEntry*)(_IMM(pe->address) << 12) + p.l1p_index;
		if (!pe->isPresent()) return (PageEntry*)~_IMM0;
		if (pe->huge_page) return pe; // 2MB

		// Level 1: PT
		pe = (PageEntry*)(_IMM(pe->address) << 12) + p.l0p_index;
		return pe;

	}

	void* Paging::operator[](stduint address) const {
		auto entry = getEntry(address);
		if (_IMM(entry) == ~_IMM0 || !entry->isPresent()) return (void*)~_IMM0;
		if (entry->huge_page) {
			stduint pgsize = 0;
			if (address >= (_IMM1 << 48)) return (void*)~_IMM0;
			pageint p = address;
			auto pe = (PageEntry*)root_level_page + p.l3p_index;
			if (!pe->isPresent()) return (void*)~_IMM0;
			pe = (PageEntry*)(_IMM(pe->address) << 12) + p.l2p_index;
			if (!pe->isPresent()) return (void*)~_IMM0;
			if (pe->huge_page) pgsize = 30; // 1GB 
			else pgsize = 21; // 2MB
			
			stduint mask = (1ULL << pgsize) - 1; 
			return (void*)((_IMM(entry->address) << 12) + (address & mask));
		}
		return (void*)((_IMM(entry->address) << 12) + (address & 0xFFF));
	}

	auto Paging::PageMap(stduint laddr, stduint paddr, stduint pgsize, stduint pgporp) -> bool {
		// pgporp is for multi-level page tables.
		if (pgsize == 12 || pgsize == 21 || pgsize == 30); else return false;
		pageint p = laddr;
		p.pg_size = pgsize;
		p.crt_level =
			pgsize == 12 ? 0 : pgsize == 21 ? 1 : 2;
		//
		const unsigned X64_MAXLEV = 4;
		// if (p.crt_level > X64_MAXLEV - 1) return (PageEntry*)~_IMM0;
		auto pe = root_level_page + p.l3p_index;
		const stduint indexes[X64_MAXLEV - 1] = {
			p.l2p_index, p.l1p_index, p.l0p_index
		};
		stduint lev = X64_MAXLEV - 1 - p.crt_level;

		PageEntry* path[X64_MAXLEV];
		PageEntry* tables[X64_MAXLEV];
		tables[3] = (PageEntry*)root_level_page;
		path[3] = pe;

		for0(i, lev) {
			const stduint index = indexes[i];
			if (!pe->isPresent()) {
				if (!(pgporp & PGPROP_present)) return true; // Already unmapped
				if (!uni_default_allocator) return false;
				auto new_pg = uni_default_allocator->allocate(0x1000, 0x1000);
				if (!new_pg) return false;
				MemSet(pe, 0, sizeof(PageEntry));
				MemSet(new_pg, 0, 0x1000);
				pe->address = _IMM(new_pg) >> 12;
				pe->present = _TEMP 1;
				pe->writable = true; // pgporp & PGPROP_writable;
				pe->user_access = true; // pgporp& PGPROP_user_access;
				pe->global = nil; // pgporp& PGPROP_global; (only for leaf)
			}
			const stduint addr = _IMM(pe->address) << 12;
			tables[2 - i] = (PageEntry*)addr;
			pe = tables[2 - i] + index;
			path[2 - i] = pe;
		}

		if (pgporp & PGPROP_present) {
			// proc huge page
			if (p.crt_level) {
				pe->huge_page = true;
			}
			pe->address = _IMM(paddr) >> 12;
			pe->present = _TEMP 1;
			pe->writable = !!(pgporp & PGPROP_writable);
			pe->user_access = !!(pgporp & PGPROP_user_access);
			pe->global = !!(pgporp & PGPROP_global);
		} else {
			pe->present = 0;
			pe->address = 0;
			for (stduint i = p.crt_level; i < X64_MAXLEV - 1; i++) {
				bool empty = true;
				PageEntry* table = tables[i];
				for (int j = 0; j < 512; j++) {
					if (table[j].present) {
						empty = false;
						break;
					}
				}
				if (empty && uni_default_allocator) {
					uni_default_allocator->deallocate(table, 0x1000);
					path[i + 1]->present = 0;
					path[i + 1]->address = 0;
				} else {
					break;
				}
			}
		}
		return true;
	}
	
	auto Paging::Map(stduint ln_address, stduint ph_address, stduint length, stduint pgsize, stduint pgporp) -> bool {
		if (pgsize == 12 || pgsize == 21 || pgsize == 30); else return false;
		if (!length) return true;
		stduint alignmask = (_IMM1 << pgsize) - 1;
		ph_address &= ~alignmask;
		if (ln_address >= ln_address + length) return false;
		length += ln_address & alignmask;
		ln_address &= ~alignmask;
		const stduint unit = alignmask + 1;
		if (pgporp & PGPROP_weak) do {
			auto entry = getEntry(ln_address);
			if (_IMM(entry) == ~_IMM0 || !entry->isPresent())
				PageMap(ln_address, ph_address, pgsize, pgporp);
			ln_address += unit, ph_address += unit;
			length -= minof(unit, length);
		} while (length);
		else do {
			PageMap(ln_address, ph_address, pgsize, pgporp);
			ln_address += unit, ph_address += unit;
			length -= minof(unit, length);
		} while (length);
		return true;
	}

	auto Paging::Unmap(stduint ln_address, stduint ph_address, stduint length, stduint pgsize) -> bool {
		return Map(ln_address, ph_address, length, pgsize, 0);
	}

	void Paging::Reset() {
		root_level_page = (uni::PageEntry*)uni_default_allocator->allocate(0x1000, 0x1000);
		MemSet(root_level_page, 0, 0x1000);
	}

#endif

#if defined(_ARC_x86) || defined(_ARC_x64)

	stduint MemCopyP(void* dest, Paging& pg_d, const void* sors, Paging& pg_s, size_t length)
	{
		stduint offset_d = _IMM(dest) & 0xFFF;
		stduint offset_s = _IMM(sors) & 0xFFF;

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
			
			stduint pg_offset_d = offset_d;
			stduint pg_offset_s = offset_s;
			stduint phy_d = 0;
			stduint phy_s = 0;

			if (crtpage_d->huge_page) {
				stduint mask = (1ULL << (sizeof(stduint) == 4 ? 22 : 21)) - 1; 
				pg_offset_d = _IMM(dest) & mask;
				phy_d = (_IMM(crtpage_d->address) << 12) + pg_offset_d;
			} else {
				phy_d = (_IMM(crtpage_d->address) << 12) + pg_offset_d;
			}

			if (crtpage_s->huge_page) {
				stduint mask = (1ULL << (sizeof(stduint) == 4 ? 22 : 21)) - 1;
				pg_offset_s = _IMM(sors) & mask;
				phy_s = (_IMM(crtpage_s->address) << 12) + pg_offset_s;
			} else {
				phy_s = (_IMM(crtpage_s->address) << 12) + pg_offset_s;
			}

			stduint page_bound_d = crtpage_d->huge_page ? (sizeof(stduint) == 4 ? 0x400000 : 0x200000) : 0x1000;
			stduint page_bound_s = crtpage_s->huge_page ? (sizeof(stduint) == 4 ? 0x400000 : 0x200000) : 0x1000;

			stduint unit = length;
			MIN(unit, page_bound_d - pg_offset_d);
			MIN(unit, page_bound_s - pg_offset_s);

			MemCopyN((void*)(phy_d), (void*)(phy_s), unit);
			cast<char*>(dest) += unit;
			cast<char*>(sors) += unit;
			length -= unit;
			ret += unit;
			offset_d = _IMM(dest) & 0xFFF;
			offset_s = _IMM(sors) & 0xFFF;
		}
		return ret;
	}

	stduint StrCopyP(char* dest, Paging& pg_d, const char* sors, Paging& pg_s, size_t length)
	{
		stduint offset_d = _IMM(dest) & 0xFFF;
		stduint offset_s = _IMM(sors) & 0xFFF;

		stduint ret = 0;

		while (length) {
			auto crtpage_d = pg_d.getEntry(_IMM(dest));
			auto crtpage_s = pg_s.getEntry(_IMM(sors));

			if (_IMM(crtpage_d) == ~_IMM0 || !crtpage_d->isPresent()) return 0;
			if (_IMM(crtpage_s) == ~_IMM0 || !crtpage_s->isPresent()) return 0;
			
			stduint pg_offset_d = offset_d;
			stduint pg_offset_s = offset_s;
			stduint phy_d = 0;
			stduint phy_s = 0;

			if (crtpage_d->huge_page) {
				stduint mask = (1ULL << (sizeof(stduint) == 4 ? 22 : 21)) - 1; 
				pg_offset_d = _IMM(dest) & mask;
				phy_d = (_IMM(crtpage_d->address) << 12) + pg_offset_d;
			} else {
				phy_d = (_IMM(crtpage_d->address) << 12) + pg_offset_d;
			}

			if (crtpage_s->huge_page) {
				stduint mask = (1ULL << (sizeof(stduint) == 4 ? 22 : 21)) - 1;
				pg_offset_s = _IMM(sors) & mask;
				phy_s = (_IMM(crtpage_s->address) << 12) + pg_offset_s;
			} else {
				phy_s = (_IMM(crtpage_s->address) << 12) + pg_offset_s;
			}

			stduint page_bound_d = crtpage_d->huge_page ? (sizeof(stduint) == 4 ? 0x400000 : 0x200000) : 0x1000;
			stduint page_bound_s = crtpage_s->huge_page ? (sizeof(stduint) == 4 ? 0x400000 : 0x200000) : 0x1000;

			stduint unit = length;
			MIN(unit, page_bound_d - pg_offset_d);
			MIN(unit, page_bound_s - pg_offset_s);
			
			MemCopyN((void*)(phy_d), (void*)(phy_s), unit);
			for0(i, unit) {
				char ch = cast<char*>(phy_s)[i];
				if (!ch) {
					cast<char*>(phy_d)[i] = nil;// zero-terminate of ASCIZ
					return ret += i;
				}
				cast<char*>(phy_d)[i] = ch;
			}
			dest += unit, sors += unit;
			length -= unit;
			ret += unit;
			offset_d = _IMM(dest) & 0xFFF;
			offset_s = _IMM(sors) & 0xFFF;
		}
		if (1) {
			auto crtpage_d = pg_d.getEntry(_IMM(dest));
			if (_IMM(crtpage_d) == ~_IMM0 || !crtpage_d->isPresent()) return 0;
			stduint pg_offset_d = _IMM(dest) & 0xFFF;
			stduint phy_d = 0;
			if (crtpage_d->huge_page) {
				stduint mask = (1ULL << (sizeof(stduint) == 4 ? 22 : 21)) - 1; 
				pg_offset_d = _IMM(dest) & mask;
			}
			phy_d = (_IMM(crtpage_d->address) << 12) + pg_offset_d;
			treat<char>(phy_d) = nil;// zero-terminate of ASCIZ
		}
		return ret;
	}

#endif

}
