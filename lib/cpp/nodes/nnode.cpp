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
#include "../../../inc/c/nnode.h"
#include <new>

namespace uni {

#define tmpl(...) __VA_ARGS__ NnodeChain

	tmpl(stduint)::Length() const {
		_TODO return nil;
	}

	tmpl()::Nchain(bool defa_free) {
		root_node = nullptr;
		func_free = nullptr;
		extn_field = nil;
		if (defa_free) this->func_free = NnodeHeapFreeSimple;
	}

	tmpl()::~Nchain() {
		NnodesRelease(root_node, func_free);
	}
	
	tmpl(Nnode*)::Append(pureptr_t addr, bool onleft, Nnode* nod) {
		Nnode* tmp;
		if (nod) {
			tmp = NnodeInsert(nod, onleft ? 0 : 1, extn_field);
			if (root_node == nod && onleft) root_node = tmp;
		}
		else {
			tmp = NnodeInsert(onleft ? root_node : Youngest(), onleft ? 0 : 1, extn_field);
			if (!root_node) root_node = tmp;
		}
		tmp->offs = addr;
		return tmp;
	}

	tmpl(Nnode*)::Remove(Nnode* nod) {
		return NnodeRelease(nod, func_free);
	}
	
	tmpl(bool)::Exchange(Nnode* idx1, Nnode* idx2) {
		xchg(idx1->type, idx2->type);
		xchgptr(idx1->offs, idx2->offs);
		for0(i, extn_field) {
			byte& b1 = *NnodeGetExtnField(idx1);
			byte& b2 = *NnodeGetExtnField(idx2);
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
		
		// 1case .@@
		// "a" "+++" "b", "a" "++"-'+'-"b"
		if (idx == 0)
		{
			Nnode* newd = NnodeAppend(inp, 1, StrHeap(inp->addr + width), tok_symbol, sizeof(TnodeField));
			Letvar(tf, TnodeField* const, NnodeGetExtnField(newd));
			tf->col += width;
			((char*)inp->offs)[width] = 0;
			return NNODE_DIVSYM_HEAD;
		}
		// 2case @@.
		// "a" "+++" "b", "a"-'++'-"+" "b"
		if (idx + width == slen) {
			Nnode* newd = NnodeAppend(inp, 0, inp->offs, tok_symbol, sizeof(TnodeField));
			Letvar(tf, TnodeField* const, NnodeGetExtnField(newd));
			tf->col += width;
			inp->addr = StrHeap(inp->addr + idx);
			tf->col += slen - width;
			newd->addr[slen - width] = 0;
			return NNODE_DIVSYM_TAIL;
		}
		// 3else @.@
		// "a" "+++*" "b", "a"-'++'-"+"-'*'-"b"
		else
		{
			char* tmpaddr_mid = StrHeapN(inp->addr + idx, width);

			Nnode* newleft = NnodeAppend(inp, 0, inp->offs, tok_symbol, sizeof(TnodeField));
			Nnode* newright = NnodeAppend(inp, 1, StrHeap(inp->addr + idx + width), tok_symbol, sizeof(TnodeField));

			((TnodeField*)NnodeGetExtnField(inp))->col += idx;

			((TnodeField*)NnodeGetExtnField(newright))->col =
				((TnodeField*)NnodeGetExtnField(inp))->col
				+ width;

			inp->offs = tmpaddr_mid;
			((char*)newleft->offs)[idx] = 0;
			return NNODE_DIVSYM_MIDD;
		}
	}

#undef tmpl
}
