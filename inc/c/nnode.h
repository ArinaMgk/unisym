// ASCII C/C++ TAB4 CRLF
// Docutitle: Node for Nested / Tree / Binary
// Codifiers: @dosconio: ~ 20240702
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Descripts: 20240630 new design: left of eldest child points to parent, aka this->subf==this->subf->left for parent, this->left->subf==this for the eldest.
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

#ifndef _INC_NNODE
#define _INC_NNODE

#include "stdinc.h"
#include "node.h"
#include "dnode.h"
#include "ustring.h"

#if defined(_INC_CPP)
#include "../cpp/trait/ArrayTrait.hpp"
#include "../cpp/trait/IterateTrait.hpp"
#include "algorithm/sort.h"
#include "algorithm/search.h"
namespace uni {
extern "C" {
#endif

typedef struct Nnode
{
	// Dnode
	struct Nnode* next;
	union { char* addr; void* offs; };
	union { struct Nnode* left, * pare; };
	union { stduint type; stduint lens; };
	//
	struct Nnode* subf;// sub-first item
	//
#ifdef _INC_CPP

	Nnode* Head() {
		Nnode* crt = this; while (crt) {
			if (isEldest(crt)) return crt;
			crt = crt->left;
		}
		return 0;// panic
	}

	Nnode* Tail() {
		Nnode* crt = this; while (crt->next && (crt = crt->next)); return crt;
	}

	inline Nnode* getLeft() { return isEldest() ? nullptr : left; }
	Nnode* getParent() {
		Nnode* crt = this;
		while (!crt->isEldest()) crt = crt->left;
		if (!crt->pare) return nullptr;
		if (crt == crt->pare->subf) return crt->pare;
		return nullptr;
	}

	byte* GetExtnField() { return getExfield(self); }
	TnodeField* GetTnodeField() { return (TnodeField*)GetExtnField(); }
	inline mag_node_t& GetMagnoField() { return *(mag_node_t*)(GetExtnField()); }

	Nnode* ReheapString(const char* str) {
		srs(this->addr, StrHeap(str));
		return this;
	}

	static bool isEldest(Nnode* nod);
	bool isEldest() { return isEldest(this); }

#endif
} Nnode, nnode;// {comb with nnode}

inline static int Nnode_isEldest(Nnode* nod) {
	if (!nod->left) return 1; // root node
	return nod->left->subf == nod;
}

inline static Nnode* Nnode_getLeft(Nnode* nod) {
	return Nnode_isEldest(nod) ? 0 : nod->left;
}

Nnode* NnodeInsert(Nnode* nod, pureptr_t offs, stduint typlen, stduint extn_field, int direction_right);

// Set a part of nnode as the sub of a nnode. If subtail is null, this is for all the right part of the nnode string. If only one item, keep subhead and subtail same.
Nnode* NnodeBlock(Nnode* nod, Nnode* subhead, Nnode* subtail);

// If freefunc is not null, free for memory will be defined by user, or just free the node block. In the direction of right.
Nnode* NnodeRemove(nnode* nod, _tofree_ft freefunc);

// If freefunc is not null, free for memory will be defined by user, or just free the node block. In the direction of right.
void NnodesRelease(nnode* nod, _tofree_ft freefunc);

void NnodeHeapFreeSimple(pureptr_t inp);

#if defined(_INC_CPP)
} // C++ Area

//{TODO} nchn[1][1] <=> root_node->next->subf->next

#undef Pref
#define Pref(x) NNODE_DIVSYM_##x
enum NNODE_DIVSYM_RETYPE {
	Pref(NONE), Pref(HEAD), Pref(TAIL), Pref(MIDD), Pref(ERRO)
};
#undef Pref

class Nchain
{
protected:
	Nnode* root_node;
	//
	virtual stduint Length() const;

	inline Nnode* New() {
		return (Nnode*)zalc(sizeof(Nnode) + extn_field);
	}
	
public:
	_tofree_ft func_free;
	stduint extn_field;
	//
	Nchain(bool defa_free = false); ~Nchain();
	// 
	inline Nnode* Youngest() const {
		Nnode* res = root_node;
		if (!res) return nullptr;
		while (res->next) res = res->next;
		return res;
	}
	/*

	virtual pureptr_t Locate(stduint idx) const;

	Nnode* LocateNode(stduint idx) const;// (D)Node Special

	virtual stduint Locate(pureptr_t p_val, bool fromRight) const;

	// Index First One
	inline Nnode* LocateNode(pureptr_t content) {
		return LocateNode(Locate(content, false));
	}
	*/
	//
	stduint Count() { return Length(); }
	//
	Nnode* Append(pureptr_t addr, bool onleft, Nnode* nod = 0);
	inline Nnode* Append(pureptr_t addr, bool onleft, Nnode* nod, stduint typlen) {
		Nnode* res = Append(addr, onleft, nod);
		res->lens = typlen;
		return res;
	}
	template<typename type0> Nnode* Append(const type0& obj) {
		return Append((pureptr_t)&obj, false);
	}
	Nnode* Append(Tnode* tn) {
		Nnode* res = Append(StrHeap(tn->addr), false, 0, tn->type);
		res->GetTnodeField()->col = tn->col;
		res->GetTnodeField()->row = tn->row;
		return res;
	}
	//
	Nnode* Remove(Nnode* nod);
	//
	bool Exchange(Nnode* idx1, Nnode* idx2);
	//
	Nnode* Root() const { return root_node; }
	Nnode*& RootRef() { return root_node; }

	//[consider this not practical] void Sort();

	// void Index(void* content);

	inline Nnode* Adopt(Nnode* thisn, Nnode* subhead, Nnode* subtail = (Nnode*)None, bool go_func = true) {
		if (go_func) thisn->type = tok_func;
		return NnodeBlock(thisn, subhead, subtail);
	}

	NNODE_DIVSYM_RETYPE DivideSymbols(Nnode* inp, stduint width, stduint idx);
	
	//{TEMP} only for Nnode AREA
	Nnode* Receive(Nnode* insnod, Dchain* dnod, bool onleft = false);

	inline static void Traversal(Nnode* nod, void (traversal)(Nnode& nod, stduint nest), stduint nest = 0) {
		for (; nod; nod = nod->next) {
			traversal(*nod, nest);
			if (nod->subf) Traversal(nod->subf, traversal, nest + 1);
		}
	}
	inline void Traversal(void (traversal)(Nnode& nod, stduint nest), stduint nest = 0) {  Traversal(root_node, traversal, nest); }

};

using NnodeChain = Nchain;
using TnodeChain = Dchain;

// Process in order for C
// - { } Block
// - ( ) Parens and [ ] Brackets
// -  ;  Comma
// Used by
// - Magice
// - COTLAB
class NestedParseUnit {
private:
	NnodeChain* chain;
public:
	enum class Method {
		CPL, CPP = CPL,
		COT, Python = COT,
	};

	Method method = Method::CPL;

	NodeChain* TokenOperatorGroupChain;
	NestedParseUnit(const TnodeChain& tchain, NodeChain* TOGCChain, stduint extn_fielen = sizeof(mag_node_t));// will destructure TnodeChain
	~NestedParseUnit();
	NnodeChain* GetNetwork() { return chain; }
	// Process: () [] and operators
	bool Parse() {
		//return NnodeParse(chain->Root(), chain);
		bool state = ParseParen(chain->Root(), chain);
		if (state) state = ParseOperator(chain->Root(), chain);
		if (!state) {
			chain->~Nchain();
		}
		return state;
	}

	void ParseBlockStatements_CPL(Nnode* beg_nod); void ParseBlockStatements_CPL() { ParseBlockStatements_CPL(chain->Root()); }


protected:
	// Process current and child : () and []
	bool ParseParen(Nnode* tnod, NnodeChain* chain, bool merge_parensd = true);
	
	// [OLD] Process: () [] and operators
	bool NnodeParse(Nnode* tnod, NnodeChain* chain, bool merge_parensd = true);
	// Process Current Operator
	bool ParseOperator(Nnode* pare, NnodeChain* nc);
};



struct TokenOperator {
	const char* idnop;
	const char* ident;
	_tok_bindfunc_t bindfn;
};

class TokenOperatorGroup {
public:
	TokenOperator* operators;
	stduint count;
	bool left_to_right;
	stduint condition;// "?:" is ternary conditional operator
	//
	TokenOperatorGroup(TokenOperator* ops, stduint cnt, bool ltr = true, stduint cond = 2) {
		operators = ops; count = cnt; left_to_right = ltr; condition = cond;
	}
};// usually arranged by levels for its array/chain

const char* StrIndexOperator(const char* str, uni::TokenOperator** operators, size_t count, bool left_to_right);

} // C++ END
#else
#include "ustdbool.h"

typedef struct NnodeChain_t {
	Nnode* root_node;
	stduint extn_field;
	_tofree_ft func_free;
	_tocomp_ft func_comp;
} nchain_t, Nchain;

size_t NnodeCount(const Nnode* chn);//{TODO} count in the next and subf direction

inline static Nnode* NnodeNew(Nchain* chn) {
	return (Nnode*)zalc(sizeof(Nnode) + chn->extn_field);
}

//{TODO}
void   NchainInit(nchain_t* chain);
void   NchainDrop(nchain_t* chain);
void   NchainSort(nchain_t* chain);
Nnode* NchainAppend(nchain_t* chn, pureptr_t addr, bool onleft, Nnode* nod);
Nnode* NchainLocateNode(nchain_t* chn, stduint idx);
// {OCCUPY POSITION} void   NnodeChainAdapt(nchain_t* chn, Nnode* root, Nnode* last, stdint count_dif);

#endif
#endif // !_INC_NNODE
