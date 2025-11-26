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

#include "../../../inc/c/ustring.h"
#include "../../../inc/c/nnode.h"
#include <new>

#define ON_LEFT  0
#define ON_RIGHT 1

static stduint NnodeWalkN(uni::Nnode* nnod)
{
	stduint n = 0;
	uni::Nnode* tmpnode = nnod;
	if (!nnod) return 0;
	while (tmpnode) {
		n++;
		if (tmpnode->subf) n += NnodeWalkN(tmpnode->subf);
		tmpnode = tmpnode->next;
	}
	return n;
}

namespace uni {

	bool Nnode::isEldest(Nnode* nod) {
		if (!nod->left) return true; // root node
		return nod && nod->left->subf == nod;
	}


#define tmpl(...) __VA_ARGS__ NnodeChain

	tmpl(stduint)::Length() const {
		return NnodeWalkN(root_node);
	}

	tmpl()::Nchain(bool defa_free) {
		root_node = nullptr;
		func_free = nullptr;
		extn_field = nil;
		if (defa_free) this->func_free = NnodeHeapFreeSimple;
	}

	tmpl()::~Nchain() {
		if (root_node) NnodesRelease(root_node, func_free);
		root_node = nullptr;
		func_free = nullptr;
		extn_field = nil;
	}

	tmpl(Nnode*)::Append(pureptr_t addr, bool onleft, Nnode* nod) {
		Nnode* tmp;
		if (nod) {
			tmp = NnodeInsert(nod, 0, 0, extn_field, onleft ? 0 : 1);
			if (root_node == nod && onleft) root_node = tmp;
		}
		else {
			tmp = NnodeInsert(onleft ? root_node : Youngest(), 0, 0, extn_field, onleft ? 0 : 1);
			if (!root_node) root_node = tmp;
		}
		tmp->offs = addr;
		return tmp;
	}

	tmpl(Nnode*)::Remove(Nnode* nod) {
		Nnode* nex = NnodeRemove(nod, func_free);
		asrtequ(root_node, nod) = nex;
		return nex;
	}

	tmpl(bool)::Exchange(Nnode* idx1, Nnode* idx2) {
		xchg(idx1->type, idx2->type);
		xchgptr(idx1->offs, idx2->offs);
		xchgptr(idx1->subf, idx2->subf);
		for0(i, extn_field) {
			byte& b1 = *getExfield(*idx1);
			byte& b2 = *getExfield(*idx2);
			xchg(b1, b2);
		}
		return true;
	}

	tmpl(NNODE_DIVSYM_RETYPE)::DivideSymbols(Nnode* inp, stduint width, stduint idx)
	{
		stduint slen = StrLength((const char*)inp->offs);
		// i+++j, +++, "+" idx(2)width(1)slen(3)
		if (slen == 0 || idx + width > slen) return NNODE_DIVSYM_ERRO;
		// 0case .
		if (slen == width) return NNODE_DIVSYM_NONE;// assert (idx zo 0)

		MAX(extn_field, sizeof(TnodeField));

		// 1case .@@
		// "a" "+++" "b", "a" "++"-'+'-"b"
		if (idx == 0)
		{
			Nnode* newd = NnodeInsert(inp, StrHeap(inp->addr + width), tok_symbol, extn_field, ON_RIGHT);
			Letvar(tf, TnodeField* const, getExfield(*newd));
			tf->row = inp->GetTnodeField()->row;
			tf->col = inp->GetTnodeField()->col + width;
			((char*)inp->offs)[width] = 0;
			return NNODE_DIVSYM_HEAD;
		}
		// 2case @@.
		// "a" "+++" "b", "a"-'++'-"+" "b"
		if (idx + width == slen) {
			Nnode* newd = NnodeInsert(inp, inp->offs, tok_symbol, extn_field, ON_LEFT);
			if (root_node == inp) root_node = newd;
			Letvar(tf, TnodeField* const, getExfield(*newd));
			tf->row = inp->GetTnodeField()->row;
			tf->col = inp->GetTnodeField()->col;
			inp->addr = StrHeap(inp->addr + idx);
			inp->GetTnodeField()->col += slen - width;
			newd->addr[slen - width] = 0;
			return NNODE_DIVSYM_TAIL;
		}
		// 3else @.@
		// "a" "+++*" "b", "a"-'++'-"+"-'*'-"b"
		else
		{
			char* tmpaddr_mid = StrHeapN(inp->addr + idx, width);

			Nnode* newleft = NnodeInsert(inp, inp->offs, tok_symbol, extn_field, ON_LEFT);
			Nnode* newright = NnodeInsert(inp, StrHeap(inp->addr + idx + width), tok_symbol, extn_field, ON_RIGHT);
			newleft->GetTnodeField()->row = newright->GetTnodeField()->row = inp->GetTnodeField()->row;
			newleft->GetTnodeField()->col = inp->GetTnodeField()->col;
			newright->GetTnodeField()->col = inp->GetTnodeField()->col + idx + width;
			inp->GetTnodeField()->col += idx;

			if (root_node == inp) root_node = newleft;

			(TnodeGetExtnField(*inp))->col += idx;

			(TnodeGetExtnField(*newright))->col =
				(TnodeGetExtnField(*inp))->col
				+ width;

			inp->offs = tmpaddr_mid;
			((char*)newleft->offs)[idx] = 0;
			return NNODE_DIVSYM_MIDD;
		}
	}

	tmpl(Nnode*)::Receive(Nnode* insnod, DnodeChain* dnod, bool onleft) {
		Dnode* crt = dnod->Root();
		if (crt) do {
			(insnod = Append(crt->offs, onleft, insnod))->type = crt->type;
		} while (crt = crt->next);
		dnod->func_free = nullptr;
		dnod->~DnodeChain();
		return insnod;
	}

#undef tmpl
}
