// ASCII C++ TAB4 CRLF
// Attribute: <ArnCovenant> <Env> <bin^%> <CPU()> [Allocation]
// LastCheck: 
// AllAuthor: @dosconio
// ModuTitle: Linear Token Node for C++
// Depend-on: StrHeapN.c
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

#include "../../../inc/cpp/cinc"
#include "../../../inc/c/ustring.h"
#include "../../../inc/cpp/cinc"
#include "../../../inc/cpp/tnode"
#include <new>

namespace uni {

#define tmpl(...) __VA_ARGS__ TnodeChain

	tmpl()::TnodeChain(bool need_free) : DnodeChain(need_free), root_node((Tnode*&)DnodeChain::root_node), last_node((Tnode*&)DnodeChain::last_node) {
	    
	}

	tmpl()::~TnodeChain() {
		if (nullptr == root_node) return;
		Tnode* next = (Tnode*)root_node;
		while (next)
		{
			root_node = next->next;
			Remove(next, false);
			next = root_node;
		}
	}

	tmpl(Tnode*)::Append(const void* addr, stduint contlen, stduint typ, stduint row, stduint col) {
		if (!contlen) return 0;
		Tnode* tmp = zalcof(Tnode);
		new (tmp) Tnode(0);
		tmp->offs = StrHeapN((const char *)addr, contlen);
		tmp->type = typ;
		tmp->next = nullptr;
		tmp->left = (Tnode*)last_node;
		tmp->col = col;
		tmp->row = row;
		node_count++;
		if (nullptr == root_node)
		{
			last_node = root_node = tmp;
		}
		// if (aflaga.autosort)
		// else if (_node_compare)
		else
		{
			last_node = last_node->next = tmp;
		}
		return tmp;
	}

	tmpl(Tnode*)::Remove(Tnode* nod, bool systematic) {
		if (!nod) return 0;
		Tnode* ret = nod->next;
		if (systematic) {
			if (root_node == nod) root_node = nod->next;
			if (last_node == nod) last_node = nod->left;
			if (nod->left) nod->left->next = nod->next;
			if (nod->next) nod->next->left = nod->left;
		}
		dchainfree(nod,);
		return ret;
	}

#undef tmpl
}
