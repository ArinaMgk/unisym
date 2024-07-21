// ASCII C99/C++11 TAB4 CRLF
// Docutitle: Node
// Codifiers: @ArinaMgk(~RFA03) @dosconio(20240409)
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

#ifndef _INC_NODE
#define _INC_NODE

#include "stdinc.h"
#include "algorithm/sort.h"
#include "algorithm/search.h"

#if defined(__cplusplus) || defined(_INC_CPP)
#include "../cpp/trait/ArrayTrait.hpp"
#include "../cpp/trait/IterateTrait.hpp"
namespace uni {
extern "C" {
#else	
#include "ustdbool.h"
#include "../c/trait/ArrayTrait.h"
#endif

// always pass pointer to Node but offs

typedef struct Node {
	struct Node* next;
	union {
		const char* addr;
		pureptr_t offs;
	};
#ifdef _INC_CPP
	byte* GetExtnField() { return getExfield(*this); }
	Node* ReheapString(const char* str);
#endif
} Node; // measures stdint[2]

Node* NodeInsert(Node* nod, pureptr_t txt, stduint more_field);

void NodeRemove(Node* nod, Node* left, _tofree_ft _node_freefunc);
void NodesRelease(Node* nod, Node* left, _tofree_ft _node_freefunc);

void NodeHeapFreeSimple(pureptr_t inp);

#define _MACRO_CHAIN_MEMBER \
	Node* root_node;\
	Node* last_node;\
	struct {\
		Node* midl_node;\
	} fastab;\
	stduint node_count;\
	stduint extn_field;\
	struct {\
		bool been_sorted /* `need_sort` as para of Append */;\
	} state;
// all be initialized with ZERO but been_sorted is decided by your preference.

typedef struct NodeChain_t {
	_MACRO_CHAIN_MEMBER
	_tofree_ft func_free;
	_tocomp_ft func_comp;
} chain_t;

#if defined(__cplusplus) || defined(_INC_CPP)
} // C++ Area
class Chain : public ArrayTrait, public IterateTrait {
protected:
	_MACRO_CHAIN_MEMBER
	//
	virtual stduint Length() const;
	void NodeChainAdapt(Node* root, Node* last, stdint count_dif);
	Node* Push(pureptr_t off, bool end_left = true);
	Node* New();
public:
	_tofree_ft func_free; // nullptr for not-auto sort, for `Append`
	//
	Chain(bool defa_free = false);
	~Chain();
	// ---- T: Iterate
	virtual void Iterate();
	// ---- T: Array
	//
	virtual pureptr_t Locate(stduint idx) const;
	Node* LocateNode(stduint idx) const;// (D)Node Special
	virtual stduint Locate(pureptr_t p_val, bool fromRight) const;
	//[Single-Direction]
	Node* getLeft(Node* nod, bool fromRight) const;
	// Locate First One
	inline Node* LocateNode(pureptr_t content) {
		return LocateNode(Locate(content, false));
	}
	//
	//[protected] virtual stduint   Length() const;
	stduint Count() { return Length(); }
	// 
	virtual bool Insert(stduint idx, pureptr_t dat);
	toheap Node* Append(const char* addr);
	// Priority: {nod > order > default_ends}
	Node* Append(pureptr_t addr, bool onleft, Node* nod = 0);
	template <typename type1> inline Chain& operator<<(const type1& obj) {
		Append((pureptr_t)&obj, false);
		return *this;
	}// do not pass pointers to what you what but itself!
	inline Chain& operator<<(const char* addr) {
		Append(addr);
		return *this;
	}
	//
	virtual bool Remove(stduint idx, stduint times);
	Node* Remove(Node* nod);
	Node* Remove(pureptr_t content);
	//
	virtual bool Exchange(stduint idx1, stduint idx2);
	//
	Node* Root() const { return root_node; }
	template <typename type1> inline type1& Head() {
		if (!root_node) throw "NODE ROOT EMPTY";
		return *(type1*)root_node->offs;
	}
	Node* Last() const { return last_node; }// Reference version: Tail() , which fits at least 1 item
	template <typename type1> inline type1& Tail() {
		if (!last_node) throw "NODE TAIL EMPTY";
		return *(type1*)last_node->offs;
	}

	// Pointer Operator
	Node* operator[](stdint idx) { if (idx < 0) idx += node_count; return (Node*)Locate(idx); }
	//


	// Sorted
	Chain& Sorted(_tocomp_ft Cmp_f = 0) {
		if (Cmp_f) this->Compare_f = Cmp_f;
		Sort(*this);
		state.been_sorted = true;
		return *this;
	}

	void SortByInsertion();
	
};

using NodeChain = Chain;



}
#else

// How many nodes after the node
size_t NodeCount(const Node* first);

Node* NodeNew(chain_t* chn);

void ChainInit(chain_t* chain);
void ChainDrop(chain_t* chain);
void ChainSort(chain_t* chain);
Node* ChainAppend(chain_t* chn, pureptr_t addr, bool onleft, Node* nod);
Node* ChainLocateNode(chain_t* chn, stduint idx);

inline static void NodeChainAdapt(chain_t* chn, Node* root, Node* last, stdint count_dif) {
	chn->  node_count += count_dif;
	chn->  root_node = root;
	chn->  last_node = last;
	//[Fast Table except root/last node]
	// assume 35 items, consider 33 items, 33 / 2 + 1 = 17;
	// assume 3 items ... <=> 3 / 2 = 1;
	if (chn->  node_count < 2 + 1) // root, last, and 
		chn->  fastab.midl_node = 0;
	else
		chn->fastab.midl_node = ChainLocateNode(chn, chn->node_count >> 1);
}

inline static Node* getLeft(chain_t* chn, Node* nod, bool fromRight) {
	Node* res;
	Node* crt = chn->root_node;
	if (crt) do if (crt->next == nod) {
		res = crt;
		if (!fromRight) break;
	} while (crt = crt->next);
	return res;
}

#endif

#endif
