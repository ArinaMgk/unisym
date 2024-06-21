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

#if defined(__cplusplus) || defined(_INC_CPP)
#include "../cpp/trait/ArrayTrait.hpp"
#include "../cpp/trait/IterateTrait.hpp"
#include "algorithm/sort.h"
#include "algorithm/search.h"
namespace uni {
extern "C" {
#endif

// always pass pointer to Node but offs

typedef struct Node {
	struct Node* next;
	union {
		const char* addr;
		pureptr_t offs;
	};
} Node; // measures stdint[2]

Node* NodeInsert(Node* nod, pureptr_t txt);

void NodeRemove(Node* nod, Node* left, void (*_node_freefunc)(pureptr_t ptxt));

inline static void NodeFreeSimple(pureptr_t inp) {
	Letvar(nod, Node*, inp); 
	memf(nod->addr);
}

#define _MACRO_CHAIN_MEMBER \
	Node* root_node;\
	Node* last_node;\
	struct {\
		Node* midl_node;\
	} fastab;\
	stduint node_count;\
	struct {\
		bool been_sorted /* `need_sort` as para of Append */;\
	} state;
// all be initialized with ZERO but been_sorted is decided by your preference.

#if defined(__cplusplus) || defined(_INC_CPP)
} // C++ Area
class Chain : public ArrayTrait, public IterateTrait {
protected:
	_MACRO_CHAIN_MEMBER
	//
	virtual stduint   Length() const;
	void NodeChainAdapt(Node* root, Node* last, stdint count_dif);
	template<typename type1> inline Node& Push(const type1& obj, bool end_left = true) {
		Node* new_nod = nullptr;
		if (end_left) {
			(new_nod = NodeInsert(nullptr, (pureptr_t) & obj))->next = root_node;
			NodeChainAdapt(new_nod, last_node, +1);
		}
		else {
			NodeChainAdapt(root_node, new_nod = NodeInsert(last_node, (pureptr_t) & obj), +1);
		}
		return *new_nod;
	}
public:
	void(*func_free)(void*); // nullptr for not-auto sort, for `Append`
	//
	Chain();
	~Chain();
	// ---- T: Iterate
	virtual void Iterate();
	// ---- T: Array
	//
	virtual pureptr_t Locate(stduint idx) const;
	Node* LocateNode(stduint idx) const;// (D)Node Special
	virtual stduint Locate(pureptr_t p_val, bool fromRight) const;
	virtual Node* getLeft(Node* nod, bool fromRight) const;
	//
	//[protected] virtual stduint   Length() const;
	stduint Count() { return Length(); }
	// 
	virtual bool      Insert(stduint idx, pureptr_t dat);
	toheap Node* Append(const char* addr);
	// Priority: {nod > order > default_ends}
	Node* Append(pureptr_t addr, bool onleft, Node* nod = 0);
	template <typename type1> inline Chain& operator<<(const type1& obj) {
		Append((pureptr_t)&obj, false);
		return *this;
	}
	inline Chain& operator<<(const char* addr) {
		Append(addr);
		return *this;
	}
	//
	virtual bool      Remove(stduint idx, stduint times);
	bool Remove(Node* nod);
	bool Remove(pureptr_t content);
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
	Node* operator[](stduint idx) {	return (Node*)Locate(idx); }
	//
	template <typename type1> inline type1& get(stduint idx) {
		return *(type1*)((*this)[idx]->offs);
	}


	// Sorted
	Chain& Sorted(Compare_ft Cmp_f = nullptr) {
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
#include "ustdbool.h"
#include "../c/trait/ArrayTrait.h"
typedef struct NodeChain_t {
	_MACRO_CHAIN_MEMBER
	void(*func_free)(void*);
	int (*func_comp)(pureptr_t a, pureptr_t b);
} chain_t, Chain; // measures stdint[2]

// How many nodes after the node
size_t NodeCount(const Node* first);

void ChainInit(Chain* chain);
void ChainDrop(Chain* chain);


#endif

#endif
