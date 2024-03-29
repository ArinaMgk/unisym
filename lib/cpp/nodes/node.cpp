// ASCII C++ TAB4 CRLF
// Attribute: Allocate(Need)
// LastCheck: unchecked for C++ version
// AllAuthor: @dosconio
// ModuTitle: Node for C++
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
#include "../../../inc/cpp/node"

inline static int _defa_compare(const void* a, const void* b) {
	return (char*)a - (char*)b;
}

namespace uni {

#define tmpl(...) __VA_ARGS__ NodeChain

	tmpl()::NodeChain(bool need_free) {
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

	tmpl()::~NodeChain() {
		if (nullptr == root_node) return;
		Node* next = root_node;
		while (next)
		{
			root_node = next->next;
			if (need_free_content)
				(_node_freefunc ? _node_freefunc : _memf)((void*)(free_pass_whole ? next : next->offs));
			else memf(next);
			node_count--;
			
			next = root_node;
		}
	}

	Node* NodeChain::Append(const void* addr) {
		// Node* tmp = zalcof(Node);
		// tmp->offs = addr;
		// tmp->next = nullptr;
		// node_count++;
		// if (nullptr == root_node)
		// 	last_node = root_node = tmp;
		// else
		// 	last_node = ((Node*)last_node)->next = tmp;
		return Append(addr, false);
	}

	toheap Node* NodeChain::Append(const char* addr) {
		return Append(StrHeap(addr), false);
	}

	unchecked Node* NodeChain::Append(const void* addr, bool onleft, Node* nod) {
		Node* tmp = zalcof(Node);
		tmp->offs = addr;
		tmp->next = 0; // nullptr;
		node_count++;

		Node* last = root_node;
		Node* crt = root_node;
		int on_decresing_order = !little_endian;

		if (nod) while (crt != nod && crt->next) {
			AssignParallel(last, crt, crt->next);
			if (crt != nod) {
				if (onleft) tmp->next = nod; else nod->next = tmp;
				return tmp;// out of current chain
			}
		}
		else if (!root_node)
			return last_node = root_node = tmp;
		else if (!sorted)
			return last_node = (last_node->next = tmp);
		else if (((_node_compare ? _node_compare : _defa_compare)(addr, root_node->offs) <= 0) ^ on_decresing_order)
			AssignParallel(tmp->next, root_node, tmp);// insert left
		// insert right
		else if (!_node_compare) while (crt->next && ((crt->next->offs < addr) ^ on_decresing_order))
				AssignParallel(last, crt, crt->next);
		else while (crt->next && _node_compare(crt->next->offs, addr) < 0)
				AssignParallel(last, crt, crt->next);
		if (((_node_compare ? _node_compare : _defa_compare)(crt->offs, addr) < 0) ^ on_decresing_order)
			last = crt;
		if (crt)
			(last->next = tmp)->next = crt->next;
		return tmp;
	}

#undef tmpl
}
