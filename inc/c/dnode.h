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

#if defined(_INC_CPP)
#include "../cpp/trait/ArrayTrait.hpp"
#include "../cpp/trait/IterateTrait.hpp"
#include "algorithm/sort.h"
#include "algorithm/search.h"
namespace uni {
extern "C" {
	struct TnodeField;
#else
#include "ustdbool.h"
#endif

// always pass pointer to Node but offs
struct Tnode;
typedef struct Dnode {
	struct Dnode* next;
	union { char* addr; pureptr_t offs; };
	struct Dnode* left;
	union { stduint type; stduint lens; };
#ifdef _INC_CPP

	byte* GetExtnField() { return getExfield(self); }
	TnodeField* GetTnodeField() { return (TnodeField*)GetExtnField(); }
	Tnode* GetTnode() { return (Tnode*)this; }
	Dnode* ReheapString(const char* str);

#endif
} Dnode; // measures stdint[4]

inline static byte* DnodeGetExtnField(Dnode* nod) {
	return (byte*)nod + sizeof(Dnode);
}

Dnode* DnodeInsert(Dnode* nod, pureptr_t offs, size_t typlen, stduint extn_field, int direction_right);

void DnodeRemove(Dnode* nod, _tofree_ft _dnode_freefunc);
void DnodesRelease(Dnode* first, _tofree_ft _dnode_freefunc);

void DnodeHeapFreeSimple(pureptr_t inp);

Dnode* DnodeRewind(const Dnode* any);

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
		#ifdef _INC_CPP
		
		inline bool difline(const TnodeField& nodf) {
			return row != nodf.row;
		}
		
		#endif
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
	#define TnodeGetExtnField(x) ((TnodeField*)getExfield(x))



	inline static tchain_t* StrTokenNew(dchain_t* chn) {
		chn->extn_field = sizeof(TnodeField);
		chn->func_free = DnodeHeapFreeSimple;
		return (tchain_t*)chn;
	}

#endif// ---- TOKEN NODE END

#if defined(_INC_CPP)
} // C++ Area
extern "C++" {
class Dchain : public trait::Array, public IterateTrait {
protected:
	_MACRO_DCHAIN_MEMBER
	//
	virtual stduint Length() const;
	void DnodeChainAdapt(Dnode* root, Dnode* last, stdint count_dif);
	Dnode* Push(pureptr_t off, bool end_left = true);
	Dnode* New();
public:
	_tofree_ft func_free;
	//
	Dchain(_tofree_ft fn_free = DnodeHeapFreeSimple); ~Dchain();
	// ---- T: Iterate
	virtual void Iterate();
	// ---- T: Array
	virtual pureptr_t Locate(stduint idx) const;
	Dnode* LocateNode(stduint idx) const;// (D)Node Special
	virtual stduint Locate(pureptr_t p_val, bool fromRight) const;
	// Index First One
	inline Dnode* LocateNode(pureptr_t content, _tocomp_ft cmp = 0) {
		Dnode* res = nullptr, * crt = root_node;
		if (!cmp) while (crt) {
			if ((crt->offs == content)) { res = crt; break; }
			else crt = crt->next;
		}
		else while (crt)
			if (!cmp(crt->offs, content)) { res = crt; break; }
			else crt = crt->next;
		return res;
	}
	//
	stduint Count() { return Length(); }
	//
	virtual bool Insert(stduint idx, pureptr_t dat);
	//
	Dnode* Append(pureptr_t addr, bool onleft = false, Dnode* nod = 0);

	toheap Dnode* AppendHeapstr(char* addr);
	// Priority: {nod > order > default_ends}
	template <typename type1> inline Dchain& operator<<(type1* obj) {
		Append((pureptr_t)obj, false);
		return *this;
	}


	template <typename type1> inline Dchain& operator<<(const type1& obj) {
		Append((pureptr_t)&obj, false);
		return *this;
	}
	// DO NOT << for heapping rostr
	inline Dchain& operator<<(const char* addr) {
		Append((pureptr_t)addr);
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
		// if (!root_node) throw "DNODE ROOT EMPTY";
		return *(type1*)root_node->offs;
	}
	Dnode* Last() const { return last_node; }// Reference version: Tail() , which fits at least 1 item
	template <typename type1> inline type1& Tail() {
		// if (!last_node) throw "DNODE TAIL EMPTY";
		return *(type1*)last_node->offs;
	}

	// Pointer Operator
	Dnode* operator[](stduint idx) { return (Dnode*)Locate(idx); }

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
		self.Sort();
		state.been_sorted = true;
		return *this;
	}

	void setExtnField(stduint ext) {
		extn_field = ext;
	}

protected:
	void SortByInsertion();
public:
	inline void Sort(SortMode method = SortMode::Insertion) {
		switch (method) {
		case SortMode::Insertion: SortByInsertion(); break;
		default: ::uni::Sort(self, method);
		}
	}

};
}
using DnodeChain = Dchain;

typedef void(*_tok_bindfunc_t)(DnodeChain* io);
typedef struct mag_node_t {
	stduint row, col;// same with tnode
	_tok_bindfunc_t bind;
} mag_node_t;

} // C++ END
namespace uni { extern "C" {
#endif

size_t DnodeCount(const Dnode* chn);//{TODO} count in the next direction
	
Dnode* DnodeNew(dchain_t* chn);

//{TODO}
void   DchainInit(dchain_t* chain);
void   DchainDrop(dchain_t* chain);
void   DchainSort(dchain_t* chain);
Dnode* DchainAppend(dchain_t* chn, pureptr_t addr, bool onleft, Dnode* nod);
Dnode* DchainLocateNode(dchain_t* chn, stduint idx);
void   DnodeChainAdapt(dchain_t* chn, Dnode* root, Dnode* last, stdint count_dif);

inline static Dnode* Dnode_getLeft(Dnode* nod) {
	return nod->left;
}

#ifdef _INC_CPP
}}
#endif // _INC_CPP


#include "tnode.h"

#endif// !_INC_DNODE
