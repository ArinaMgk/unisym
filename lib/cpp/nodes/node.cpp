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
				(_node_freefunc ? _node_freefunc : _memf)((void*)next->offs);
			memf(next);
			node_count--;
			
			next = root_node;
		}
	}

	toheap Node* NodeChain::Append(const char* addr) {
		return Append(StrHeap(addr));
	}

	// nod zo yo chain
	unchecked Node* NodeChain::Append(const void* addr, bool onleft, Node* nod) {
		Node* tmp = 0;
		Node* last = 0;
		Node* crt = root_node;
		int on_decresing_order = !little_endian;
		if (nod) do {
			if (crt != nod) continue;
			tmp = NodeInsert(onleft ? last : nod, addr);
			if (!last) tmp->next = nod;
			asrtequ(onleft ? root_node : last_node, nod) = tmp;
			node_count++; return tmp;
		} while (AssignParallel(last, crt, crt->next));
		if (nod) return 0;// out of the chain
		tmp = NodeInsert(0, addr); node_count++;
		if (!root_node)
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

	void NodeChain::Remove(const void* content)	{
		if (nullptr == root_node) return;
		Node* next = root_node;
		while (next)
		{
			if (next->offs != content) continue;
			root_node = next->next;
			if (need_free_content)
				(_node_freefunc ? _node_freefunc : _memf)((void*)next->offs);
			memf(next);
			node_count--;

			next = root_node;
		}
	}

#undef tmpl
}
