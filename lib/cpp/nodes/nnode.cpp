// ASCII C++ TAB4 CRLF
// Attribute: <ArnCovenant> <Env> <bin^%> <CPU()> [Allocation]
// LastCheck: 
// AllAuthor: @dosconio
// ModuTitle: Nested Node for C++
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
#include "../../../inc/cpp/nnode"
#include <new>

namespace uni {

#define tmpl(...) __VA_ARGS__ NnodeChain

	tmpl()::NnodeChain(bool need_free) : TnodeChain(need_free) {
	}

	tmpl()::~NnodeChain() {
		if (nullptr == root_node) return;
		Nnode* next = (Nnode*)root_node;
		while (next)
		{
			root_node = next->next;
			if (need_free_content)
				(_node_freefunc ? _node_freefunc : _memf)((void*)(free_pass_whole ? next : next->offs));// ~Nnode();
			else memf(next);
			node_count--;
			next = (Nnode*)root_node;
		}
	}

	tmpl(void)::Append(const void* addr, stduint typ, stduint col, stduint row) {
		Nnode* tmp = zalcof(Nnode);
		new (tmp) Nnode;
		tmp->offs = addr;
		tmp->type = typ;
		tmp->next = nullptr;
		tmp->left = (Nnode*)last_node;
		tmp->col = col;
		tmp->row = row;
		tmp->bind = 0;
		tmp->next = tmp->subf = 0;
		node_count++;
		if (nullptr == root_node)
		{
			last_node = root_node = tmp;
		}
		// if (aflaga.autosort)
		// else if (_node_compare)
		else
		{
			last_node = ((Nnode*)last_node)->next = tmp;
		}
	}

#undef tmpl
}
