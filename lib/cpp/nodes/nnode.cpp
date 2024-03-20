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

#define tmpl(...) __VA_ARGS__ Nnode

	Nnode* Nnode::ReheapString(const char* str) {
		srs(this->addr, StrHeap(str));
		return this;
	}


#undef tmpl
#define tmpl(...) __VA_ARGS__ NnodeChain

	tmpl()::NnodeChain(bool need_free) : TnodeChain(need_free) {
	}

	tmpl()::~NnodeChain() {
		NnodesRelease(root_node, this);
	}

	tmpl(Nnode*)::Append(const void* addr, stduint typ, stduint col, stduint row) {
		Nnode* tmp = Insert(last_node, false, addr, typ);
		tmp->col = col;
		tmp->row = row;
		tmp->bind = tmp->next = tmp->subf = 0;
		return tmp;
	}

	tmpl(Nnode*)::Append(Tnode* tnod) {
		return Append(tnod->offs, tnod->type, tnod->col, tnod->row);
	}

	tmpl(Nnode*)::Insert(Nnode* insnod, bool onleft, const void* addr, stduint typ) {
		if (!insnod && node_count) return nullptr;
		Nnode* tmp = zalcof(Nnode);
		new (tmp) Nnode;
		tmp->offs = (void*)addr;
		tmp->type = typ;
		tmp->col = insnod ? insnod->col : 0;
		tmp->row = insnod ? insnod->row : 0;
		tmp->subf = 0;
		tmp->pare = stepval(insnod)->pare;
		node_count++;
		if (!insnod) return last_node = root_node = tmp;// assert(!root_node)
		if (onleft) {
			if (tmp->left = insnod->left) tmp->left->next = tmp;
			(tmp->next = insnod)->left = tmp;
			insnod->isHead(tmp);
			if (insnod == root_node) root_node = tmp;// assert(!tmp->left)
		}
		else {
			if (tmp->next = insnod->next) tmp->next->left = tmp;
			(tmp->left = insnod)->next = tmp;
			if (insnod == last_node) last_node = tmp;// assert(!tmp->next)
		}
		return tmp;
	}

	tmpl(Nnode*)::Insert(Nnode* insnod, Dnode* dnod, bool onleft) {
		return Insert(insnod, onleft, dnod->offs, dnod->type);
	}

	tmpl(Nnode*)::Receive(Nnode* insnod, DnodeChain* dnod, bool onleft) {
		Dnode* crt = dnod->Root();
		while (crt) {
			insnod = Insert(insnod, crt, onleft);
			crt = crt->next;
		}
		dnod->Onfree(0, false);
		dnod->~DnodeChain();
		return insnod;
	}

	tmpl(Nnode*)::Adopt(Nnode* thisn, Nnode* subhead, Nnode* subtail, bool go_func) {
		bool found = false;
		if (subtail == (Nnode*)(~(stduint)0)) subtail = subhead;
		if (!subhead) return 0;
		if (thisn->subf) return 0;//{TODO} Remove each in current chain
		//
		if (go_func) thisn->type = tok_func;
		if (subhead->left == subtail)// for empty parens and parend "(" ")"
			return thisn;
		// Above: "(" no right, subhead zo ")"; ")" no left, subtail zo "("

		Nnode* crt;
		crt = subhead;
		if (subhead->isHead(thisn) && this)
			if (root_node == subhead) root_node = thisn;
			else if (last_node == subtail) last_node = subhead->left;
		do if (crt == subtail) {
			found = true;
			crt->pare = thisn;
			break;
		} while ((crt->pare = thisn) && (crt = crt->next));
		// if (!found) return 0; ... throw ...
		// ----
		Nnode* paraleft, * paralext = stepval(subtail)->next;
		AssignParallel(paraleft, subhead->left, 0);
		thisn->subf = subhead;
		// [nod] [] ... [sub1] [sub2] ... []

		if (paraleft) paraleft->next = paralext;
		if (paralext) paralext->left = paraleft;
		asserv(subtail)->next = 0;
		return thisn;
	}

	tmpl(Nnode*)::NnodeRelease(Nnode* nod, NnodeChain* nc, bool systematic) {
		Nnode* ret = nod->next;
		asrtequ(nc->root_node, nod) = nod->next;
		asrtequ(nc->last_node, nod) = nod->left;
		if (systematic) {
			asserv (nod->left)->next = nod->next;
			asserv (nod->next)->left = nod->left;
		}   
		if (nod->subf) NnodesRelease(nod->subf, nc);
		if (asrtand(nod->pare)->subf == nod)
			nod->pare->subf = nod->next;
		dchainfree(nod, nc->);
		return ret;
	}
	tmpl(void)::NnodesRelease(Nnode* nods, NnodeChain* nc) {
		if (0 == nods) return;
		while (nods = NnodeRelease(nods, nc, false));
	}

	tmpl(NNODE_DIVSYM_RETYPE)::DivideSymbols(Nnode* inp, stduint width, stduint idx)
	{
		stduint slen = StrLength((const char*)inp->offs);
		// i+++j, +++, "+" idx(2)width(1)slen(3)
		if (slen == 0 || idx + width > slen) return NNODE_DIVSYM_ERRO;
		// 0case .
		if (slen == width) return NNODE_DIVSYM_NONE;// assert (idx zo 0)
		
		// 1case .@@
		// "a" "+++" "b", "a" "++"-'+'-"b"
		if (idx == 0)
		{
			Nnode* newd = Insert(inp, false, StrHeap((char*)inp->offs + width), tok_symbol);
			newd->col += width;
			((char*)inp->offs)[width] = 0;
			return NNODE_DIVSYM_HEAD;
		}
		// 2case @@.
		// "a" "+++" "b", "a"-'++'-"+" "b"
		if (idx + width == slen) {
			Nnode* newd = Insert(inp, true, inp->offs, tok_symbol);
			inp->addr = StrHeap(inp->addr + idx);
			inp->col += slen - width;
			newd->addr[slen - width] = 0;
			return NNODE_DIVSYM_TAIL;
		}
		// 3else @.@
		// "a" "+++*" "b", "a"-'++'-"+"-'*'-"b"
		else
		{
			char* tmpaddr_mid = StrHeapN((char*)inp->offs + idx, width);
			Nnode* newleft = Insert(inp, true, inp->offs, tok_symbol);
			Nnode* newright = Insert(inp, false, StrHeap((char*)inp->offs + idx + width), tok_symbol);
			inp->col += idx;
			newright->col = inp->col + width;
			inp->offs = tmpaddr_mid;
			((char*)newleft->offs)[idx] = 0;
			return NNODE_DIVSYM_MIDD;
		}
	}

#undef tmpl
}
