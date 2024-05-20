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
			if (onleft && !last) tmp->next = nod;
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
		Node* crt = root_node, *nex, *las = 0;
		if (crt) do {
			nex = crt->next;
			if (crt->offs != content) continue;
			if (crt == root_node) root_node = nex;
			if (crt == last_node) last_node = las;
			if (las) las->next = nex;// else this is Root
			if (need_free_content)
				(_node_freefunc ? _node_freefunc : _memf)((void*)crt->offs);
			memf(crt);
			node_count--;
		} while (AssignParallel(las, crt, nex));
	}

	void NodeChain::Remove(Node* nod) {
		if (nullptr == root_node) return;
		Node* crt = root_node, * nex, * las = 0;
		if (crt) do {
			nex = crt->next;
			if (crt != nod) continue;
			if (crt == root_node) root_node = nex;
			if (crt == last_node) last_node = las;
			if (las) las->next = nex;// else this is Root
			if (need_free_content)
				(_node_freefunc ? _node_freefunc : _memf)((void*)crt->offs);
			memf(crt);
			node_count--;
			return;
		} while (AssignParallel(las, crt, nex));
	}

	static int NodeSortDefault(const void* addr0, const void* addr1) {
		return (stduint)addr0 - (stduint)addr1;
	}
	void NodeChain::SortByInsertion() {
		int (*node_compare)(const void* addr0, const void* addr1) = NodeSortDefault;
		if (_node_compare) node_compare = _node_compare;
		Node* crt = Root();
		if (!crt) return;
		Node* last = crt, *next = crt->next;
		while (crt = next) {
			next = crt->next;

			if (little_endian ? (node_compare(last->offs, crt->offs) > 0) :
				(node_compare(last->offs, crt->offs) < 0))
			{
				// a_bit_lower crt a_bit_bigger...last
				// Dnode can directly make use of left item
				// end with crt<=Root or lower<crt<=bigger
				Node* a_bit_bigger = Root(), * a_bit_lower = 0;
				if (little_endian? (node_compare(crt->offs, a_bit_bigger->offs) <= 0):
					(node_compare(crt->offs, a_bit_bigger->offs) >= 0)) 
				{
					Append(crt->offs, true, a_bit_bigger);
					Remove(crt);
					continue;
				}
				AssignParallel(a_bit_lower, a_bit_bigger, a_bit_bigger->next);
				while (a_bit_bigger != crt) {
					if (little_endian ? (node_compare(a_bit_lower->offs, crt->offs) < 0 &&
						node_compare(crt->offs, a_bit_bigger->offs) <= 0):
						(node_compare(a_bit_lower->offs, crt->offs) > 0 &&
							node_compare(crt->offs, a_bit_bigger->offs) >= 0))
					{
						Append(crt->offs, false, a_bit_lower);
						Remove(crt);
						break;
					}
					AssignParallel(a_bit_lower, a_bit_bigger, a_bit_bigger->next);
				}
			}
			else last = crt;
		}
	}

	void NodeChain::SortBySelection() {
		int (*node_compare)(const void* addr0, const void* addr1) = NodeSortDefault;
		if (_node_compare) node_compare = _node_compare;
		stduint ptr = 0;
		Node* crt = Root();
		if (crt) do {
			Node* clim_one = crt;
			Node* crtcrt = crt->next;
			if (crtcrt) do {
				if (little_endian ? (node_compare(crtcrt->offs, clim_one->offs) < 0) :
					node_compare(crtcrt->offs, clim_one->offs) > 0)
					clim_one = crtcrt;
			} while (crtcrt = crtcrt->next);
			if (clim_one != crt) Exchange(crt, clim_one);
		} while (crt = crt->next);
	}

#undef tmpl
}
