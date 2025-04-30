// ASCII C/C++ TAB4 CRLF
// Docutitle: Node for Double-Direction Double-Field Linear Chain
// Codifiers: @dosconio: ~ 20240701
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

#include "../../../inc/cpp/dnode"
#include "../../../inc/cpp/cinc"
#include "../../../inc/c/ustring.h"
#include "../../../inc/cpp/cinc"

namespace uni {
#define tmpl(...) __VA_ARGS__ Dchain

	Dnode* Dnode::ReheapString(const char* str) {
		srs(this->addr, StrHeap(str));
		return this;
	}

	tmpl(void)::DnodeChainAdapt(Dnode* root, Dnode* last, stdint count_dif) {
		node_count += count_dif;
		root_node = root;
		last_node = last;
		//[Fast Table except root/last node]
		// assume 35 items, consider 33 items, 33 / 2 + 1 = 17;
		// assume 3 items ... <=> 3 / 2 = 1;
		if (node_count < 2 + 1) // root, last, and 
			fastab.midl_node = nullptr;
		else
			fastab.midl_node = LocateNode(node_count >> 1);
	}

	Dchain::Dchain(_tofree_ft fn_free) {
		root_node = nullptr;
		last_node = nullptr;
		fastab.midl_node = nullptr;
		node_count = nil;
		extn_field = nil;
		func_free = nullptr;
		state.been_sorted = true;// empty chain
		func_free = fn_free;
	}

	Dchain::~Dchain() {
		Remove(0, Length());
	}

	tmpl(Dnode*)::New() {
		return (Dnode*)zalc(sizeof(Dnode) + extn_field);
	}

	// impl Iterate for Chain
	tmpl(void)::Iterate() {
		if (_iterate_datas)
			delete[] _iterate_datas;
		_iterate_lim = Length();
		_iterate_datas = new pureptr_t[_iterate_lim];
		_iterate_crt = nil;
		Dnode* crt = Root();
		for0(i, _iterate_lim) {
			_iterate_datas[i] = (pureptr_t)crt;
			crt = crt->next;
		}
	}

	// impl Array for 
	tmpl(pureptr_t)::Locate(stduint idx) const {
		Dnode* crt = Root();
		if (idx) do; while ((crt = crt->next) && (--idx));
		return (pureptr_t)(idx ? nullptr : crt);
	}
	tmpl(Dnode*)::LocateNode(stduint idx) const {
		Dnode* crt = Root();
		if (crt) do; while ((idx--) && (crt = crt->next));
		return crt; // if not found, crt is nullptr
	}
	//
	tmpl(stduint)::Locate(pureptr_t p_val, bool fromRight) const {
		// Left: Linked, Right: Iterated
		return nil;//{TODO}
	}
	//
	tmpl(stduint)::Length() const {
		return node_count;
	}
	// 
	tmpl(bool)::Insert(stduint idx, pureptr_t dat) {
		if (!idx) {
			root_node = DnodeInsert(root_node, dat, nil, extn_field, 0/*ON_LEFT*/);
		}
		else {
			auto len = Count();
			if (idx > len) return false;
			else if (idx == len) {
				last_node = DnodeInsert(last_node, dat, nil, extn_field, 1/*ON_RIGHT*/);
			}
			else {
				DnodeInsert(operator[](idx - 1), dat, nil, extn_field, 1/*ON_RIGHT*/);
			}
		}
		node_count++;
		return true;
	}
	// ---- DnodeAppendX.cpp ----
	// ---- DnodeRemoveX.cpp ----
	//
	tmpl(bool)::Exchange(stduint idx1, stduint idx2) {
		Dnode& n1 = *(Dnode*)Locate(idx1);
		Dnode& n2 = *(Dnode*)Locate(idx2);
		if (idx1 != idx2) {
			xchgptr(n1.offs, n2.offs);
			xchg(n1.type, n2.type);
		}
		return true;
	}


	// pass coff!
	tmpl(void)::SortByInsertion() {
		setcmp(self);
		Dnode* crt = Root();
		if (!crt || !crt->next) return;
		Dnode* next = crt->next;
		while (crt = next) {
			next = crt->next;
			if (cmp(crt->left, crt) > 0)
			{
				pureptr_t content = crt->offs;
				Dnode* crtcrt = crt->left->left;
				while (crtcrt && cmp(crtcrt, crt) > 0)
					crtcrt = crtcrt->left;
				if (!crtcrt) {
					Insert(0, content);
					Root()->type = crt->type;
				}
				else {
					DnodeInsert(crtcrt, content, crt->type, extn_field, 1/*ON_RIGHT*/);
					node_count++;
				}
				DnodeRemove(crt, 0);// 0 to skip release
				node_count--;
			}
		}
	}

	// Special AREA

	bool DnodeChain::Match(void* off, stduint typ) {
		Dnode* nod = Root();
		if (nod) do {
			if (nod->offs == off && nod->type == typ)
				return true;
		} while (nod = nod->next);
		return false;
	}

#undef tmpl
}
