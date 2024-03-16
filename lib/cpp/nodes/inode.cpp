// ASCII C++ TAB4 CRLF
// Attribute: <ArnCovenant> <Env> <bin^%> <CPU()> [Allocation]
// LastCheck: 
// AllAuthor: @dosconio
// ModuTitle: Identifier Node for C++
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
#include "../../../inc/cpp/inode"

namespace uni {

#define tmpl(...) __VA_ARGS__ InodeChain

	tmpl()::InodeChain(bool need_free) : NodeChain(need_free) {
		
	}

	tmpl()::~InodeChain() {
		if (nullptr == root_node) return;
		Inode* next = (Inode*)root_node;
		while (next)
		{
			root_node = next->next;
			chainfree(next);
			next = (Inode*)root_node;
		}
	}

	tmpl(void)::Append(const void* addr, const void* data, stduint typ, stduint prop) {
		Inode* tmp = zalcof(Inode);
		tmp->offs = addr;
		tmp->data = data;
		tmp->type = typ;
		tmp->property = prop;
		tmp->next = nullptr;
		node_count++;
		if (nullptr == root_node)
		{
			last_node = root_node = tmp;
		}
		// if (aflaga.autosort)
		// else if (_node_compare)
		else
		{
			last_node = ((Inode*)last_node)->next = tmp;
		}
	}

#undef tmpl
}

