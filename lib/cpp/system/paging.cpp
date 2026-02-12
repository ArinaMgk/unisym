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

	// return PD address
	PageEntry* PageDirectory::Index() { return (PageEntry*)_IMM(this); }
	// pointer to array of pointers of pages
	PageEntry* PageTable::Index(Paging& pg2) {
		// auto a = (PageEntry*)(_IMM(getParent(pg2).Index()[getID()].address) << 12);
		auto b = pg2.refEntry((_IMM(this) << (12 + 10)) | 0);// aka entry of first item in PT
		// if (a != b) plogerro("PageTable::Index this=%[x] a=%[x], b=%[x]", this, a, b);
		return b;
	}

	// pointer to entry in the PD
	PageEntry* PageTable::getEntry(Paging& pg2) {
		// auto a = &getParent(pg2).Index()[getID()];
		auto b = pg2.refEntry((_IMM(this) << (12 + 10)) | 1);
		// if (a != b) plogerro("PageTable::getEntry this=%[x] a=%[x], b=%[x]", this, a, b);
		return b;
	}
	// position self address in PT, pointer to a real page
	PageEntry* Page::getEntry(Paging& pg2) {
		// auto a = &getParent().Index(pg2)[getID()];
		auto b = pg2.refEntry((_IMM(this) << 12) | 0);
		// if (a != b) plogerro("Page::getEntry this=%[x] a=%[x], b=%[x]", this, a, b);
		return b;
	}

	PageEntry* Paging::refEntry(pageint p) {
		if (p.crt_level > 1) return (PageEntry*)~_IMM0;
		auto pe = (PageEntry*)root_level_page + p.l1p_index;
		stduint lev = 1 - p.crt_level;
		for0(i, lev) {
			const stduint index = p.l0p_index;
			if (!pe->isPresent()) return (PageEntry*)~_IMM0;
			const stduint addr = _IMM(pe->address) << 12;
			pe = (PageEntry*)addr + index;
		}
		return pe;
	}



	PageDirectory& PageTable::getParent(Paging& pg2) const {
		return *pg2.root_level_page;// not _IMM(this) >> 10
	}
	PageTable& Page::getParent() const {
		return *(PageTable*)(_IMM(this) >> 10);
	}

	

	bool PageTable::isPresent(Paging& pg2) const {
		// auto a = getParent(pg2)[getID()].getEntry(pg2);
		auto b = pg2.refEntry((_IMM(this) << (12 + 10)) | 1);
		// if (a != b) plogerro("PageTable::isPresent this=%[x] a=%[x], b=%[x]", this, a, b);
		return b->isPresent();
	}
	bool Page::isPresent(Paging& pg2) const {
		return pg2.refEntry((_IMM(this) << 12) | 1)->isPresent() &&
			pg2.refEntry((_IMM(this) << 12) | 0)->isPresent();
	}

	void Paging::setMode(PageTable& l1p, bool present, bool writable, bool user_but_superv) {
		// if not present, allocate it then set property.
		PageEntry& v = *l1p.getParent(self)[l1p.getID()].getEntry(self);
		if (!l1p.isPresent(self)) {
			v.address = _IMM(_physical_allocate(0x1000)) >> 12;
			MemSet((void*)(_IMM(v.address) << 12), 0, 0x1000);
		}
		v.present = present;
		v.writable = writable;
		v.user_access = user_but_superv;
	}
	void Paging::setMode(Page& l0p, bool present, bool writable, bool user_but_superv, stduint link_to_phy) {
		// if not present, allocate it then set property.
		if (!l0p.getParent().isPresent(self)) {
			setMode(l0p.getParent(), present, writable, user_but_superv);
		}
		l0p.getEntry(self)->address = link_to_phy >> 12;
		l0p.getEntry(self)->present = present;
		l0p.getEntry(self)->writable = writable;
		l0p.getEntry(self)->user_access = user_but_superv;
	}
	
	void Paging::Reset() {
		root_level_page = (uni::PageDirectory*)_physical_allocate(0x1000);
		MemSet(root_level_page, 0, 0x1000);
	}
	void Paging::Reset(Paging& pg_another) {
		// use 2-level paging (0x400 x 0x400 x 0x1000)
		root_level_page = (uni::PageDirectory*)_physical_allocate(0x1000);
		MemSet(root_level_page, 0, 0x1000);
		for0(i, _NUM_pd_table_entries) {
			PageEntry* p1entry = (*pg_another.root_level_page)[i].getEntry(pg_another);
			if (p1entry->isPresent()) {
				// PageMap(self, )
				//{TODO}
			}
		}
	}

	// ---↓


	void* Paging::operator[](stduint address) const {
		stduint id_l1p = address; id_l1p >>= 12 + 10; id_l1p &= 0x3FF; // index of page table
		auto l1p = *(*root_level_page)[id_l1p].getEntry(*(Paging*)this);// level-1 page
		if (!l1p.isPresent()) return (void*)~_IMM0;
		//
		stduint id_l0p = address; id_l0p >>= 12; id_l0p &= 0x3FF;// index of page
		auto l0p = *(*root_level_page)[id_l1p][id_l0p].getEntry(*(Paging*)this);// level-0 page
		if (!l0p.isPresent()) return (void*)~_IMM0;
		return (void*)((_IMM(l0p.address) << 12) + (address & 0xFFF));
	}

	PageEntry* Paging::getEntry(stduint address) const {
		stduint id_l1p = address; id_l1p >>= 12 + 10; id_l1p &= 0x3FF; // index of page table
		auto l1p = *(*root_level_page)[id_l1p].getEntry(*(Paging*)this);// level-1 page
		if (!l1p.isPresent()) return (PageEntry*)~_IMM0;
		//
		stduint id_l0p = address; id_l0p >>= 12; id_l0p &= 0x3FF;// index of page
		return (*root_level_page)[id_l1p][id_l0p].getEntry(*(Paging*)this);// level-0 page
	}

	static void PageMap(Paging& pg2, stduint address, stduint physical_address, bool writable, bool user_but_superv) {
		pg2.setMode(*pg2.IndexPage(address), true, writable, user_but_superv, physical_address);
	}

	// [ ge | page | page | pa ]
	bool Paging::Map(stduint ln_address, stduint physical_address, stduint length, bool writable, bool user_but_superv) {
		physical_address &= ~_IMM(0xFFF);// assert !(physical_address % 0x1000)
		if (ln_address >= ln_address + length) return false;
		length += ln_address & _IMM(0xFFF);
		ln_address &= ~_IMM(0xFFF);
		while (length) {
			stduint unit = 0x1000;
			PageMap(self, ln_address & ~_IMM(0xFFF), physical_address & ~_IMM(0xFFF), writable, user_but_superv);
			MIN(unit, length);
			ln_address += unit;
			physical_address += unit;
			length -= unit;
		}
		return true;
	}
	bool Paging::MapWeak(stduint ln_address, stduint physical_address, stduint length, bool writable, bool user_but_superv) {
		physical_address &= ~_IMM(0xFFF);// assert !(physical_address % 0x1000)
		if (ln_address >= ln_address + length) return false;
		length += ln_address & _IMM(0xFFF);
		ln_address &= ~_IMM(0xFFF);
		while (length) {
			stduint unit = 0x1000;
			auto entry = getEntry(ln_address);
			if (_IMM(entry) == ~_IMM0 || !entry->isPresent()) {
				PageMap(self, ln_address, physical_address, writable, user_but_superv);
			}
			MIN(unit, length);
			ln_address += unit;
			physical_address += unit;
			length -= unit;
		}
		return true;
	}

	// 20250730
	stduint MemCopyP(void* dest, Paging& pg_d, const void* sors, Paging& pg_s, size_t length)
	{
		stduint offset_d = _IMM(dest) & 0xFFF;
		stduint offset_s = _IMM(sors) & 0xFFF;

		Page* crtpage_d = pg_d.IndexPage(_IMM(dest));
		Page* crtpage_s = pg_s.IndexPage(_IMM(sors));
		stduint ret = 0;

		while (length) {
			if (!crtpage_d->isPresent(pg_d)) {
				plogerro(" at %s(dest %[32H], %[32H], sors %[32H], %[32H], length %[u])",
					__FUNCIDEN__, dest, pg_d.root_level_page, sors, pg_s.root_level_page, length
				);
				plogerro("MemCopyP: dest page %[32H] is not present. rest %[u]B",
					crtpage_d, length
				);
				return 0;
			}
			if (!crtpage_s->isPresent(pg_s)) {
				plogerro("MemCopyP: sors page is not present");
				return 0;
			}
			stduint phy_d = _IMM(crtpage_d->getEntry(pg_d)->address) << 12;
			phy_d += offset_d;
			stduint phy_s = _IMM(crtpage_s->getEntry(pg_s)->address) << 12;
			phy_s += offset_s;
			stduint unit = 0x1000;
			MIN(unit, length);
			MIN(unit, 0x1000 - offset_d);
			MIN(unit, 0x1000 - offset_s);
			MemCopyN((void*)(phy_d), (void*)(phy_s), unit);
			// ploginfo("MemCopyP: %[32H] -> %[32H] (%d bytes)", phy_d, phy_s, unit);
			cast<char*>(dest) += unit;
			cast<char*>(sors) += unit;
			length -= unit;
			ret += unit;
			offset_d = _IMM(dest) & 0xFFF;
			offset_s = _IMM(sors) & 0xFFF;
			crtpage_d = pg_d.IndexPage(_IMM(dest));
			crtpage_s = pg_s.IndexPage(_IMM(sors));
		}
		return ret;
	}
	stduint StrCopyP(char* dest, Paging& pg_d, const char* sors, Paging& pg_s, size_t length)
	{
		stduint offset_d = _IMM(dest) & 0xFFF;
		stduint offset_s = _IMM(sors) & 0xFFF;

		Page* crtpage_d = pg_d.IndexPage(_IMM(dest));
		Page* crtpage_s = pg_s.IndexPage(_IMM(sors));
		stduint ret = 0;

		while (length) {
			if (!crtpage_d->isPresent(pg_d)) return 0;
			if (!crtpage_s->isPresent(pg_s)) return 0;
			stduint phy_d = _IMM(crtpage_d->getEntry(pg_d)->address) << 12;
			phy_d += offset_d;
			stduint phy_s = _IMM(crtpage_s->getEntry(pg_s)->address) << 12;
			phy_s += offset_s;
			stduint unit = 0x1000;
			MIN(unit, length);
			MIN(unit, 0x1000 - offset_d);
			MIN(unit, 0x1000 - offset_s);
			MemCopyN((void*)(phy_d), (void*)(phy_s), unit);
			// ploginfo("StrCopyP: %[32H] -> %[32H] (%d bytes)", phy_d, phy_s, unit);
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
			crtpage_d = pg_d.IndexPage(_IMM(dest));
			crtpage_s = pg_s.IndexPage(_IMM(sors));
		}
		if (1) {
			if (!crtpage_d->isPresent(pg_d)) return 0;
			stduint phy_d = _IMM(crtpage_d->getEntry(pg_d)->address) << 12;
			phy_d += offset_d;
			treat<char>(phy_d) = nil;// zero-terminate of ASCIZ
		}
		return ret;
	}

#elif defined(_ARC_x64)// IA32e

	auto Paging::Map(stduint linear_address, stduint physical_address, stduint length, stduint pgsize, stduint pgporp) -> bool {
		return _TODO false;
	}

#endif
}
