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
#include <new>

namespace uni {

#define tmpl(...) __VA_ARGS__ InodeChain

	tmpl()::InodeChain(bool need_free) : NodeChain(need_free), root_node((Inode*&)NodeChain::root_node), last_node((Inode*&)NodeChain::last_node) {
	}

	tmpl()::~InodeChain() {
		if (nullptr == root_node) return;
		Inode* crt = root_node;
		while (crt = Remove(crt, false));
		root_node = nullptr;
	}

	tmpl(Inode*)::Remove(Inode* inod, bool systematic) {
		Inode* left = 0, * crt = root_node;
		if (!inod) return 0;
		Inode* ret_next = inod->next;
		if (systematic) {
			if (inod != crt)
			do if (crt->next == inod) {
				left = crt;
				break;
			} while (crt = crt->next);
			if (left) left->next = ret_next;
			if(root_node == inod) root_node = ret_next;
			if(last_node == inod) last_node = left;
		}
		if (need_free_content)
			(_node_freefunc ? _node_freefunc : _memf)
				((void*)(free_pass_whole ? inod : inod->data));
		else {
			memf(inod->offs);
			memf(inod);
		}
		node_count--;

		return ret_next;
	}

	tmpl(void)::Remove(const char* content) {
		Remove(Index(content));
	}

	tmpl(void)::Append(const char* addr, const void* data, stduint typ, bool readonly, bool typekeep) {
		Inode* tmp = zalcof(Inode);
		new (tmp) Inode(StrHeap(addr), data, typ, readonly, typekeep);
		node_count++;
		last_node = (root_node ? last_node->next : root_node) = tmp;
	}

#undef tmpl

	Inode* InodeChain::Index(const char* content) {
		if (!root_node) return 0;
		Inode* crt = root_node;
		do if (crt->addr && !StrCompare(content, crt->addr))
			return crt;
		while (crt = crt->next);
		return 0;
	}

	bool InodeChain::Update(const char* iden, void* data, size_t typ, bool readonly, bool typekeep) {
		Inode* crt;
		if (crt = Index(iden))
			if (crt->readonly) return false;
			else if (crt->typekeep && (typ != crt->type)) return false;
			else Remove(iden);
		Append(iden, data, typ, readonly, typekeep);
		return true;
	}
}

