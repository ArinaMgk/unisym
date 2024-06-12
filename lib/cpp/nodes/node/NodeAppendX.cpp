// ASCII C99/C++11 TAB4 CRLF
// Docutitle: Node
// Codifiers: @ArinaMgk(~RFA03) @dosconio(20240409)
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
#include "../../../../inc/c/node.h"
#include "../../../../inc/cpp/cinc"
#include "../../../../inc/c/ustring.h"
#include "../../../../inc/cpp/cinc"
namespace uni {

	toheap Node* Chain::Append(const char* addr) {
		return Append((pureptr_t)StrHeap(addr), false);
	}
	Node* Chain::Append(pureptr_t addr, bool onleft, Node* nod) {
		const bool need_sort = nullptr != Compare_f;
		const bool tmp_dir = false; // from left
		Node* new_nod = 0;
		int rstate;// return state

		if (nod) {
			new_nod = NodeInsert(onleft ? getLeft(nod, tmp_dir) : nod, addr);
			Node* const ro = !root_node || onleft && (nod == root_node) ? new_nod : root_node;
			Node* const la = !last_node || !onleft && (nod == last_node) ? new_nod : last_node;
			NodeChainAdapt(ro, la, +1);
		} 
		else if (!root_node) {
			// assert last_node and !node_count
			new_nod = NodeInsert(nullptr, addr);
			NodeChainAdapt(new_nod, new_nod, +1);
		}
		else if (need_sort) {
			setcmp(*this);
			// assert been_sorted
			if (!state.been_sorted) 
				Sort(*this); 
			state.been_sorted = true;
			Node tmp_nod = { .next = nullptr, .offs = addr };
			if (cmp((pureptr_t)&tmp_nod, (pureptr_t)root_node) <= 0) { // less than any
				return &Push(*(pureptr_t*)addr);
			}
			Node* crt = root_node;
			while (cmp((pureptr_t)&tmp_nod, (pureptr_t)crt) > 0 && (crt = crt->next));
			if (!crt) {
				new_nod = NodeInsert(last_node, addr);
				NodeChainAdapt(root_node, new_nod, +1);
			}
			else {
				new_nod = NodeInsert(getLeft(crt, tmp_dir), addr);
				NodeChainAdapt(root_node, last_node, +1);
			}
		}
		else {
			if (onleft)
				Push(*(pureptr_t*)addr);
			else Push(*(pureptr_t*)addr, false);
		}
		return new_nod;
	}
}
