// ASCII C/C++ TAB4 CRLF
// Docutitle: Dnode for Double-Direction Double-Field Linear Chain
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

#include "../../../../inc/cpp/unisym"
#include "../../../../inc/c/dnode.h"
#include "../../../../inc/cpp/cinc"
#include "../../../../inc/c/ustring.h"
#include "../../../../inc/cpp/cinc"


namespace uni {
	Dnode* Dchain::Push(pureptr_t off, bool end_left) {
		Dnode* new_nod = nullptr;
		if (end_left) {
			(new_nod = DnodeInsert(nullptr, off, nil, extn_field, 1/*ON_RIGHT*/))->next = root_node;
			DnodeChainAdapt(new_nod, last_node, +1);
		}
		else {
			DnodeChainAdapt(root_node, new_nod = DnodeInsert(last_node, off, nil, extn_field, 1/*ON_RIGHT*/), +1);
		}
		return new_nod;
	}

	Dnode* Dchain::Append(const char* addr) {
		return Append((pureptr_t)addr, false);
	}
	Dnode* Dchain::Append(char* addr) {
		return Append((pureptr_t)StrHeap(addr), false);
	}
	Dnode* Dchain::Append(pureptr_t addr, bool onleft, Dnode* nod) {
		const bool need_sort = nullptr != Compare_f;
		Dnode* new_nod = 0;

		if (nod) {
			new_nod = DnodeInsert(onleft ? nod->left : nod, addr, nil, extn_field, 1/*ON_RIGHT*/);
			Dnode* const ro = !root_node || onleft && (nod == root_node) ? new_nod : root_node;
			Dnode* const la = !last_node || !onleft && (nod == last_node) ? new_nod : last_node;
			DnodeChainAdapt(ro, la, +1);
		}
		else if (!root_node) {
			// assert last_node and !node_count
			new_nod = DnodeInsert(nullptr, addr, nil, extn_field, 1/*ON_RIGHT*/);
			DnodeChainAdapt(new_nod, new_nod, +1);
		}
		else if (need_sort) {
			setcmp(self);
			// assert been_sorted
			if (!state.been_sorted)
				Sort(self);
			state.been_sorted = true;
			Dnode tmp_nod; // = { .next = nullptr, .offs = addr };
			{
				tmp_nod.next = nullptr;
				tmp_nod.offs = addr;
			}
			if (cmp((pureptr_t)&tmp_nod, (pureptr_t)root_node) <= 0) { // less than any
				return Push(addr);
			}
			Dnode* crt = root_node;
			while (cmp((pureptr_t)&tmp_nod, (pureptr_t)crt) > 0 && (crt = crt->next));
			if (!crt) {
				new_nod = DnodeInsert(last_node, addr, nil, extn_field, 1/*ON_RIGHT*/);
				DnodeChainAdapt(root_node, new_nod, +1);
			}
			else {
				new_nod = DnodeInsert(crt->left, addr, nil, extn_field, 1/*ON_RIGHT*/);
				DnodeChainAdapt(root_node, last_node, +1);
			}
		}
		else new_nod = Push(addr, onleft);
		return new_nod;
	}
}
