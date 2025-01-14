// ASCII C99 TAB4 CRLF
// AllAuthor: @dosconio
// ModuTitle: Double-directions Node
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


#include "../../../inc/c/dnode.h"

enum { ON_LEFT = 0, ON_RIGHT = 1 };

static Dnode* DnodePush(dchain_t* chn, pureptr_t off, bool end_left) {
	Dnode* new_nod = NULL;
	if (end_left) {
		(new_nod = DnodeInsert(NULL, off, nil, chn->extn_field, ON_RIGHT))->next = chn->root_node;
		DnodeChainAdapt(chn, new_nod, chn->last_node, +1);
	}
	else {
		DnodeChainAdapt(chn, chn->root_node,
			new_nod = DnodeInsert(chn->last_node, off, nil, chn->extn_field, ON_RIGHT),
			+1);
	}
	return new_nod;
}

Dnode* DchainAppend(dchain_t* chn, pureptr_t addr, bool onleft, Dnode* nod) {
	// const bool need_sort = Compare_f;
	Dnode* new_nod = 0;
	if (nod) {
		new_nod = DnodeInsert(onleft ? nod->left : nod, addr, nil, chn->extn_field, ON_RIGHT);
		Dnode* const ro = !chn->root_node || onleft && (nod == chn->root_node) ? new_nod : chn->root_node;
		Dnode* const la = !chn->last_node || !onleft && (nod == chn->last_node) ? new_nod : chn->last_node;
		DnodeChainAdapt(chn, ro, la, +1);
	}
	else if (!chn->root_node) {
		// assert last_node and !node_count
		new_nod = DnodeInsert(NULL, addr, nil, chn->extn_field, ON_RIGHT);
		DnodeChainAdapt(chn, new_nod, new_nod, +1);
	}
/*//{TODO}
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
			return Push(*(pureptr_t*)addr);
		}
		Dnode* crt = root_node;
		while (cmp((pureptr_t)&tmp_nod, (pureptr_t)crt) > 0 && (crt = crt->next));
		if (!crt) {
			new_nod = DnodeInsert(last_node, addr, nil, extn_field, 1);// ON_RIGHT
			DnodeChainAdapt(root_node, new_nod, +1);
		}
		else {
			new_nod = DnodeInsert(crt->left, addr, nil, extn_field, 1);// ON_RIGHT
			DnodeChainAdapt(root_node, last_node, +1);
		}
	}
*/
	else new_nod = DnodePush(chn, addr, onleft);
	return new_nod;
}
