// ASCII C/C++ TAB4 CRLF
// Docutitle: Node for Simple Linear Chain
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

#include "../../../inc/c/node.h"
#include "../../../inc/cpp/cinc"
#include "../../../inc/c/ustring.h"
#include "../../../inc/cpp/cinc"

namespace uni {

	Node* Node::ReheapString(const char* str) {
		srs(this->addr, StrHeap(str));
		return this;
	}
	
	void Chain::NodeChainAdapt(Node* root, Node* last, stdint count_dif) {
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

	Node* Chain::New() {
		return (Node*)zalc(sizeof(Node) + extn_field);
	}

	Chain::Chain(bool defa_free) {
		root_node = nullptr;
		last_node = nullptr;
		fastab.midl_node = nullptr;
		node_count = nil;
		extn_field = nil;
		func_free = nullptr;
		state.been_sorted = true;// empty chain
		if (defa_free) this->func_free = NodeHeapFreeSimple;
	}

	Chain::~Chain() {
		Remove(0, Length());
	}

	// impl Iterate for Chain
	void Chain::Iterate() {
		if (_iterate_datas)
			delete[] _iterate_datas;
		_iterate_lim = Length();
		_iterate_datas = new pureptr_t[_iterate_lim];
		_iterate_crt = nil;
		Node* crt = Root();
		for0(i, _iterate_lim) {
			_iterate_datas[i] = (pureptr_t)crt;
			crt = crt->next;
		}
	}

	// impl Array for 
	pureptr_t Chain::Locate(stduint idx) const {
		Node* crt = Root();
		if (idx) do; while ((crt = crt->next) && (--idx));
		return (pureptr_t)(idx ? nullptr : crt);
	}
	Node* Chain::LocateNode(stduint idx) const {
		Node* crt = Root();
		if (crt) do; while ((idx--) && (crt = crt->next));
		return crt; // if not found, crt is nullptr
	}
	// 
	stduint Chain::Locate(pureptr_t p_val, bool fromRight) const {
		// Left: Linked, Right: Iterated
		return nil;//{TODO}
	}
	Node* Chain::getLeft(Node* nod, bool fromRight) const {
		// Left: Linked, Right: Iterated
		Node* res = nullptr;
		Node* crt = Root();
		if (crt) do if (crt->next == nod) {
			res = crt;
			if (!fromRight) break;
		} while (crt = crt->next);
		return res;
	}
	//
	stduint Chain::Length() const {
		return node_count;
	}
	// 
	bool    Chain::Insert(stduint idx, pureptr_t dat) {
		return true;//{TODO}
	}
	// ---- NodeAppendX.cpp ----
	// ---- NodeRemoveX.cpp ----
	//
	bool    Chain::Exchange(stduint idx1, stduint idx2) {
		Node& n1 = *(Node*)Locate(idx1);
		Node& n2 = *(Node*)Locate(idx2);
		if (idx1 != idx2) xchgptr(n1.offs, n2.offs);
		return true;
	}


	//

	// pass coff!
	_TEMP unchecked void Chain::SortByInsertion() {
		setcmp(*this);
		Node* crt = Root();
		if (!crt) return;
		Node* last = crt, * next = crt->next;
		while (crt = next) {
			next = crt->next;
			if (cmp(last->offs, crt->offs) > 0)
			{
				// a_bit_lower crt a_bit_bigger...last
				// Dnode can directly make use of left item
				// end with crt<=Root or lower<crt<=bigger
				Node* a_bit_bigger = Root(), * a_bit_lower = 0;
				if (cmp(crt->offs, a_bit_bigger->offs) <= 0)
				{
					Append(crt->offs, true, a_bit_bigger);
					NodeRemove(crt, last, 0);
					continue;
				}
				AssignParallel(a_bit_lower, a_bit_bigger, a_bit_bigger->next);
				while (a_bit_bigger != crt) {
					if ((cmp(a_bit_lower->offs, crt->offs) < 0 &&
						cmp(crt->offs, a_bit_bigger->offs) <= 0))
					{
						Append(crt->offs, false, a_bit_lower);
						NodeRemove(crt, last, 0);
						break;
					}
					AssignParallel(a_bit_lower, a_bit_bigger, a_bit_bigger->next);
				}
			}
			else last = crt;
		}
	}


}
