// ASCII C/C++ TAB4 CRLF
// Docutitle: Node for Double-Direction Double-Field Linear Chain
// Codifiers: @dosconio: ~ 20240701
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
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


/* AFLAGA {ASF, DF, FF, ZF, ONF}
- ASF : Auto sort the node by the DF
- DF  : in increasing or decreasing order
- FF  : something wrong raised
- ZF  : zero flag, if the node is empty
- ONF : order flag, if the node has and only has one node
*/

#ifndef _INC_DNODE
#define _INC_DNODE

#include "stdinc.h"// for using allocation

#if defined(__cplusplus) || defined(_INC_CPP)
#include "../cpp/trait/ArrayTrait.hpp"
#include "../cpp/trait/IterateTrait.hpp"
#include "algorithm/sort.h"
#include "algorithm/search.h"
namespace uni {
extern "C" {
#else
#include "ustdbool.h"
#include "../c/trait/ArrayTrait.h"
#endif

// always pass pointer to Node but offs
	
typedef struct Dnode {
	struct Dnode* next;
	union { char* addr; pureptr_t offs; };
	struct Dnode* left;
	union { stduint type; stduint lens; };
} Dnode; // measures stdint[4]

inline static byte* DnodeGetExtnField(Dnode* nod) {
	return (byte*)nod + sizeof(Dnode);
}

Dnode* DnodeInsert(Dnode* nod, pureptr_t offs, size_t typlen, stduint extn_field);

void DnodeRemove(Dnode* nod, _tofree_ft _dnode_freefunc);
void DnodeRelease(Dnode* first, _tofree_ft _dnode_freefunc);

inline static void DnodeHeapFreeSimple(pureptr_t inp) {
	Letvar(nod, Dnode*, inp);
	memf(nod->offs);
}

Dnode* DnodeRewind(const Dnode* any);

Dnode* DnodeAppend(Dnode* any, void* addr, size_t typlen, stduint extn_field);

inline static void DnodeFreeSimple(pureptr_t inp) {
	Letvar(nod, Dnode*, inp);
	memf(nod->addr);
}

#define _MACRO_DCHAIN_MEMBER \
	Dnode* root_node;\
	Dnode* last_node;\
	struct {\
		Dnode* midl_node;\
	} fastab;\
	stduint node_count;\
	stduint extn_field;\
	struct {\
		bool been_sorted /* `need_sort` as para of Append */;\
	} state;
// all be initialized with ZERO but been_sorted is decided by your preference.

typedef struct DnodeChain_t {
	_MACRO_DCHAIN_MEMBER
	_tofree_ft func_free;
	_tocomp_ft func_comp;
} dchain_t;

#if 1// TOKEN NODE AREA

	// typename: ((const char* []){"END", "ANY", "STR", "CMT", "DIR", "NUM", "SYM", "IDN", "SPC", "ELS"}) [first->type]

	typedef struct TnodeField // extn_field
	{
		stduint row, col;
	} TnodeField;
	// typedef Dnode Tnode;
	typedef struct Tnode {
		// Dnode Dn;
		struct Tnode* next;
		union { char* addr; pureptr_t offs; };
		struct Tnode* left;
		union { stduint type; stduint lens; };
		// TnodeField Tf;
		stduint row, col;
	} Tnode;
	typedef dchain_t tchain_t;

	// get Tnode but from Dnode
	#define TnodeGetExtnField(x) ((TnodeField*)DnodeGetExtnField(x))

	Dnode* StrTokenAppend(tchain_t* chn, const char* content, size_t contlen, size_t ttype, size_t row, size_t col);

	inline static tchain_t* StrTokenNew(dchain_t* chn) {
		chn->extn_field = sizeof(TnodeField);
		chn->func_free = DnodeHeapFreeSimple;
		return (tchain_t*)chn;
	}

	typedef struct {
		_Need_free tchain_t tchn;
		int (*getnext)(void);
		void (*seekback)(stdint chars);
		//
		stduint crtline;
		stduint crtcol;
		char* buffer, * bufptr;
	} TokenParseUnit;

#endif// ---- TOKEN NODE END

#if defined(__cplusplus) || defined(_INC_CPP)
} // C++ Area
extern "C++" {
class Dchain : public ArrayTrait, public IterateTrait {
protected:
	_MACRO_DCHAIN_MEMBER
	//
	virtual stduint Length() const;
	void DnodeChainAdapt(Dnode* root, Dnode* last, stdint count_dif);
	template<typename type1> inline Dnode& Push(const type1& obj, bool end_left = true) {
		Dnode* new_nod = nullptr;
		if (end_left) {
			(new_nod = DnodeInsert(nullptr, (pureptr_t)&obj, nil, extn_field))->next = root_node;
			DnodeChainAdapt(new_nod, last_node, +1);
		}
		else {
			DnodeChainAdapt(root_node, new_nod = DnodeInsert(last_node, (pureptr_t)&obj, nil, extn_field), +1);
		}
		return *new_nod;
	}
	inline Dnode* New() {
		return (Dnode*)zalc(sizeof(Dnode) + extn_field);
	}
public:
	void(*func_free)(void*); // nullptr for not-auto sort, for `Append`
	//
	Dchain(); ~Dchain();
	// ---- T: Iterate
	virtual void Iterate();
	// ---- T: Array
	virtual pureptr_t Locate(stduint idx) const;
	Dnode* LocateNode(stduint idx) const;// (D)Node Special
	virtual stduint Locate(pureptr_t p_val, bool fromRight) const;
	// Index First One
	inline Dnode* LocateNode(pureptr_t content) {
		return LocateNode(Locate(content, false));
	}
	//
	stduint Count() { return Length(); }
	//
	virtual bool Insert(stduint idx, pureptr_t dat);
	toheap Dnode* Append(const char* addr);
	// Priority: {nod > order > default_ends}
	Dnode* Append(pureptr_t addr, bool onleft, Dnode* nod = 0);
	template <typename type1> inline Dchain& operator<<(const type1& obj) {
		Append((pureptr_t)&obj, false);
		return *this;
	}
	inline Dchain& operator<<(const char* addr) {
		Append(addr);
		return *this;
	}
	//
	virtual bool Remove(stduint idx, stduint times);
	Dnode* Remove(Dnode* nod);
	Dnode* Remove(pureptr_t content);
	//
	virtual bool Exchange(stduint idx1, stduint idx2);
	//
	Dnode* Root() const { return root_node; }
	template <typename type1> inline type1& Head() {
		if (!root_node) throw "DNODE ROOT EMPTY";
		return *(type1*)root_node->offs;
	}
	Dnode* Last() const { return last_node; }// Reference version: Tail() , which fits at least 1 item
	template <typename type1> inline type1& Tail() {
		if (!last_node) throw "DNODE TAIL EMPTY";
		return *(type1*)last_node->offs;
	}

	// Pointer Operator
	Dnode* operator[](stduint idx) { return (Dnode*)Locate(idx); }
	//
	template <typename type1> inline type1& get(stduint idx) {
		return *(type1*)((*this)[idx]->offs);
	}

	//{TEMP} only for Dnode AREA
	// Exist : for offs
	// Match : for offs and type
	unchecked void Reverse() {
		Dnode* crt = root_node;
		if (crt) do {
			xchgptr(crt->left, crt->next);
		} while (crt = crt->left);
		xchgptr(root_node, last_node);
	}
	bool Match(void* off, stduint typ);
	// Append if-not Match
	inline void Append0Match(void* addr, stduint typ) {
		if (!Match(addr, typ)) Append(addr, typ);
	}
	// Below are compatible for old version
	// Create if not exist
	inline void Reset(void* addr, stduint typ) {
		Dnode* nod = LocateNode(addr);
		if (nod) nod->type = typ;
		else Append(addr, typ);
	}

	// Sorted
	Dchain& Sorted(Compare_ft Cmp_f = nullptr) {
		if (Cmp_f) this->Compare_f = Cmp_f;
		Sort(*this);
		state.been_sorted = true;
		return *this;
	}

	void SortByInsertion();
	
};
}
using DnodeChain = Dchain;

} // C++ END
#else

inline static Dnode* DnodeNew(dchain_t* chn) {
	return (Dnode*)zalc(sizeof(Dnode) + chn->extn_field);
}

#endif
#endif// !_INC_DNODE
