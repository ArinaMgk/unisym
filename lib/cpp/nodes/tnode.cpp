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

	tmpl()::TnodeChain() : DnodeChain(true) {
	    
	}
	
	tmpl()::TnodeChain(bool need_free) : DnodeChain(need_free) {

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
		new (tmp) Tnode;
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
			last_node = ((Tnode*)last_node)->next = tmp;
		}
		return tmp;
	}

	tmpl(void)::Remove(Tnode* nod, bool systematic) {
		if (!nod) return;
		if (systematic) {
			if (root_node == nod) root_node = nod->next;
			if (last_node == nod) last_node = nod->left;
			if (nod->left) nod->left->next = nod->next;
			if (nod->next) nod->next->left = nod->left;
		}
		if (need_free_content && !_node_freefunc) memf(nod->offs);
		if (need_free_content)
			(_node_freefunc ? _node_freefunc : _memf)((void*)(free_pass_whole ? nod : nod->offs));
		else memf(nod);
		node_count--;
	}

#undef tmpl
}
