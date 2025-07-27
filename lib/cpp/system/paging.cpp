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
		return (PageEntry*)(_IMM(getParent(pg2).Index()[getID()].address) << 12);
	}
	
	// pointer to entry in the PD
	PageEntry* PageTable::getEntry(Paging& pg2) {
		return &getParent(pg2).Index()[getID()];
	}
	// position self address in PT, pointer to a real page
	PageEntry* Page::getEntry(Paging& pg2) {
		return &getParent().Index(pg2)[getID()];
	}
	
	PageDirectory& PageTable::getParent(Paging& pg2) const {
		return *pg2.page_directory;// not _IMM(this) >> 10
	}
	PageTable& Page::getParent() const {
		return *(PageTable*)(_IMM(this) >> 10);
	}

	bool PageTable::isPresent(Paging& pg2) const {
		return getParent(pg2)[getID()].getEntry(pg2)->P;
	}
	bool Page::isPresent(Paging& pg2) const {
		return getParent().isPresent(pg2) && getParent()[getID()].getEntry(pg2)->P;
	}

	void Paging::setMode(PageTable& l1p, bool present, bool writable, bool user_but_superv) {
		// if not present, allocate it then set property.
		PageEntry& v = *l1p.getParent(self)[l1p.getID()].getEntry(self);
		if (!l1p.isPresent(self)) {
			v.address = _IMM(_physical_allocate(0x1000)) >> 12;
			MemSet((void*)(_IMM(v.address) << 12), 0, 0x1000);
		}
		v.P = present;
		v.R_W = writable;
		v.U_S = user_but_superv;
	}
	void Paging::setMode(Page& l0p, bool present, bool writable, bool user_but_superv, stduint link_to_phy) {
		// if not present, allocate it then set property.
		if (!l0p.getParent().isPresent(self)) {
			setMode(l0p.getParent(), present, writable, user_but_superv);
		}
		// if (!isPresent())
		l0p.getEntry(self)->address = link_to_phy >> 12;
		l0p.getEntry(self)->P = present;
		l0p.getEntry(self)->R_W = writable;
		l0p.getEntry(self)->U_S = user_but_superv;
	}
	
	void Paging::Reset() {
		page_directory = (uni::PageDirectory *)_physical_allocate(0x1000);
		MemSet(page_directory, 0, 0x1000);
	}

	//{unchk} ---↓


	void* Paging::operator[](stduint address) const {
		stduint id_l1p = address; id_l1p >>= 12 + 10; id_l1p &= 0x3FF; // index of page table
		auto l1p = *(*page_directory)[id_l1p].getEntry(*(Paging*)this);// level-1 page
		if (!l1p.P) return (void*)~_IMM0;
		//
		stduint id_l0p = address; id_l0p >>= 12; id_l0p &= 0x3FF;// index of page
		auto l0p = *(*page_directory)[id_l1p][id_l0p].getEntry(*(Paging*)this);// level-0 page
		if (!l0p.P) return (void*)~_IMM0;
		return (void*)(_IMM(l0p.address) << 12);
	}

	PageEntry* Paging::getEntry(stduint address) const {
		stduint id_l1p = address; id_l1p >>= 12 + 10; id_l1p &= 0x3FF; // index of page table
		auto l1p = *(*page_directory)[id_l1p].getEntry(*(Paging*)this);// level-1 page
		if (!l1p.P) return (PageEntry*)~_IMM0;
		//
		stduint id_l0p = address; id_l0p >>= 12; id_l0p &= 0x3FF;// index of page
		return (*page_directory)[id_l1p][id_l0p].getEntry(*(Paging*)this);// level-0 page
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
			if (_IMM(entry) == ~_IMM0 || !entry->P) {
				PageMap(self, ln_address, physical_address, writable, user_but_superv);
			}
			MIN(unit, length);
			ln_address += unit;
			physical_address += unit;
			length -= unit;
		}
		return true;
	}





#endif
}
