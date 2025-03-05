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

	PageDirectory* Paging::page_directory;
	
	PageEntry* PageDirectory::ref() { return (PageEntry*)_IMM(Paging::page_directory); }
	PageEntry* PageTable::ref() {
		return (PageEntry*)(_IMM(getParent().ref()[getID()].address) << 12);
	}
	PageEntry* Page::ref() {
		return &getParent().ref()[getID()];
	}


	
	bool Page::isPresent() const {
		return getParent().isPresent() && getParent().ref()[getID()].P;
	}

	void PageTable::setMode(bool present, bool writable, bool user_but_superv) {
		// if not present, allocate it then set property.
		PageEntry& v = getParent().ref()[getID()];
		if (!isPresent()) {
			v.address = _IMM(_physical_allocate(0x1000)) >> 12;
			MemSet((void*)(_IMM(v.address) << 12), 0, 0x1000);
		}
		v.P = present;
		v.R_W = writable;
		v.U_S = user_but_superv;
	}
	
	void Page::setMode(bool present, bool writable, bool user_but_superv, stduint link_to_phy) {
		// if not present, allocate it then set property.
		if (!getParent().isPresent()) {
			getParent().setMode(present, writable, user_but_superv);
		}
		// if (!isPresent())
		ref()->address = link_to_phy >> 12;
		ref()->P = present;
		ref()->R_W = writable;
		ref()->U_S = user_but_superv;
	}


	PageDirectory& PageTable::getParent() const {
		return *Paging::page_directory;// not _IMM(this) >> 10
	}
	PageTable& Page::getParent() const {
		return *(PageTable*)(_IMM(this) >> 10);
	}



	bool PageTable::isPresent() const {
		return getParent().ref()[getID()].P;
	}

	
	void Paging::Reset() {
		MemSet(page_directory, 0, 0x1000);
	}

	
#endif
}

/* CHECK EXAMPLE
	for0(i, 0x400) {
		dword v = ((dword*)kernel_paging.page_directory)[i];
		if (v) Console.OutFormat("0x%x:0x%x ", i, v);
	}
	for0(i, 0x400) {
		dword v = ((dword*)(kernel_paging.page_directory)[0][0].ref())[i];
		if (v) Console.OutFormat("0x%x:0x%x ", i, v);
	}
*/
