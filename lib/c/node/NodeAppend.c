// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ02
// AllAuthor: @ArinaMgk(till RFA03) @dosconio
// ModuTitle: Simple Node
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

#define root_node chn->root_node
#define last_node chn->last_node
#define NodeChainAdapt(a,b,c) NodeChainAdapt(chn,a,b,c)

Node* ChainAppend(chain_t* chn, pureptr_t addr, bool onleft, Node* nod)
{
	stduint extn_field = chn->extn_field;
	const bool need_sort = chn->func_comp != 0 ? true : false;
	const bool tmp_dir = false; // from left
	Node* new_nod = 0;
	int rstate;// return state

	if (nod) {
		new_nod = NodeInsert(onleft ? getLeft(chn, nod, tmp_dir) : nod, addr, extn_field);
		Node* const ro = !root_node || onleft && (nod == root_node) ? new_nod : root_node;
		Node* const la = !last_node || !onleft && (nod == last_node) ? new_nod : last_node;
		NodeChainAdapt(ro, la, +1);
	}
	else if (!root_node) {
		// assert last_node and !node_count
		new_nod = NodeInsert(nil, addr, extn_field);
		NodeChainAdapt(new_nod, new_nod, +1);
	}
	else if (need_sort) {
		setcmp(*chn);
		// assert been_sorted
		if (!chn->state.been_sorted)
			ChainSort(chn);
		chn->state.been_sorted = true;
		Node tmp_nod; // = { .next = nullptr, .offs = addr };
		{
			tmp_nod.next = nil;
			tmp_nod.offs = addr;
		}
		if (cmp((pureptr_t)&tmp_nod, (pureptr_t)root_node) <= 0) { // less than any
			// return &Push(*(pureptr_t*)addr);
			(new_nod = NodeInsert(0, addr, extn_field))->next = root_node;
			NodeChainAdapt(new_nod, last_node, +1);
			return new_nod;
		}
		Node* crt = root_node;
		while (cmp((pureptr_t)&tmp_nod, (pureptr_t)crt) > 0 && (crt = crt->next));
		if (!crt) {
			new_nod = NodeInsert(last_node, addr, extn_field);
			NodeChainAdapt(root_node, new_nod, +1);
		}
		else {
			new_nod = NodeInsert(getLeft(chn, crt, tmp_dir), addr, extn_field);
			NodeChainAdapt(root_node, last_node, +1);
		}
	}
	else {
		if (onleft) {
			// Push(*(pureptr_t*)addr);
			(new_nod = NodeInsert(0, addr, extn_field))->next = root_node;
			NodeChainAdapt(new_nod, last_node, +1);
			return new_nod;
		}
		else NodeChainAdapt(root_node, new_nod = NodeInsert(last_node, addr, extn_field), +1);//Push(*(pureptr_t*)addr, false);
	}
	return new_nod;
}
