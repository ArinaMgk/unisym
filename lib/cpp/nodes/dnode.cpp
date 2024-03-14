// ASCII C++ TAB4 CRLF
// Attribute: <ArnCovenant> <Env> <bin^%> <CPU()> [Allocation]
// LastCheck: 
// AllAuthor: @dosconio
// ModuTitle: D-D- Node for C++
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
#include "../../../inc/cpp/dnode"

namespace uni {

#define tmpl(...) __VA_ARGS__ DnodeChain

	tmpl()::DnodeChain(bool need_free) {
		node_count = 0;
		root_node = nullptr;
		last_node = nullptr;
		need_free_content = need_free;
		_node_freefunc = 0;
		sorted = false;
		little_endian = true;
		free_pass_whole = true;
		_node_compare = 0;//{MAYBE} _defa_compare, to simplify the below
	}

	tmpl()::~DnodeChain() {
		if (nullptr == root_node) return;
		Dnode* next = (Dnode*)root_node;
		while (next)
		{
			root_node = next->next;
			if (need_free_content)
				(_node_freefunc ? _node_freefunc : _memf)((void*)(free_pass_whole ? next : next->offs));
			else memf(next);
			node_count--;
			next = (Dnode*)root_node;
		}
	}

	//{TODO}
	tmpl(void)::Append(const void* addr, stduint typ) {
		Dnode* tmp = zalcof(Dnode);
		tmp->offs = addr;
		tmp->type = typ;
		tmp->next = nullptr;
		tmp->left = (Dnode*)last_node;
		node_count++;
		if (nullptr == root_node)
		{
			last_node = root_node = tmp;
		}
		// if (aflaga.autosort)
		// else if (_node_compare)
		else
		{
			last_node = ((Dnode*)last_node)->next = tmp;
		}
	}

#undef tmpl
}
