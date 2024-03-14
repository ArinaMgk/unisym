// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ22
// AllAuthor: @ArinaMgk(till RFA03) @dosconio
// ModuTitle: Double-directions Node
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

// "Dnode" have gone a class for C++.

// AFLAGA {ASF, DF, FF, ZF, ONF}
// - ASF : Auto sort the dnode by the DF
// - DF  : in increasing or decreasing order
// - FF  : something wrong raised
// - ZF  : zero flag, if the dnode is empty
// - ONF : order flag, if the dnode has and only has one dnode

#ifndef _INC_DNODE
#define _INC_DNODE

#include "host.h"// for using allocation

#ifdef _INC_CPP
class Dnode {
public:
	Dnode* next;
	const void* offs;
	Dnode* left;
	size_t type;
};
#else
typedef struct dnode
{
	union { struct dnode* next; };
	union { char* addr; void* offs; };
	union { struct dnode* left; };
	size_t type;// can be used in `free` but `index`
	// aliases: `prev` and `right` and `len` besides `left` and `next` and `type`
} dnode;// recommand using dnode. measures pointer[4]
#endif

#ifndef _INC_CPP

// default null
// return 0 for equal, 1 for greater, -1 for less
extern int (*_dnode_compare)(void* addr0, void* addr1);

// default provided by `memf()`
extern void(*_dnode_freefunc)(void*);
// 1 for pass pointer of dnode but of its `addr`. Default 0.
extern int _dnode_freepass;

// mark the head of the chain after last `NodeAppend()` , will be reset to zero after `NodeRelease()` 
extern dnode* _dnode_first;

// ---- ---- ---- ---- Structure ---- ---- ---- ---- 

// [Ordered and assume the chain is sorted] [Alloc]
// if unsorted, the new node will be appended to the tail
dnode* DnodeAppend(dnode* any, void* addr, size_t typlen);

// ---- ---- ---- ---- Auxiliary ---- ---- ---- ---- 

// [Unordered] [Alloc] Faster than `DnodeAppend()`
// <direction> `-2`from the head; `-1`left; `1`right; '2' from the tail 
dnode* DnodeInsert(dnode* nod, void* addr, size_t typlen, int direction);

// [Unordered]
dnode* DnodeBind(dnode* left, dnode* nod, dnode* right);

// ---- ---- ---- ---- Sort and Seek ---- ---- ---- ----

// [Ordered]
dnode* DnodeSort(dnode* any);

// [Unordered]
dnode* DnodeRewind(const dnode* any);

// [Unordered]
size_t DnodeCount(const dnode* any);

// [Unordered] [FailFlag:FoundNot/OptionNonexist]
// <direction> `-2`from the head; `-1`left; `1`right; '2' from the tail 
dnode* DnodeIndex(const dnode* any, void* addr, stdint* ref_span, int direction);

// ---- ---- ---- ---- Destruction ---- ---- ---- ---- 

// [Unordered] [Alloc]
void DnodeRemove(dnode* some);

// [Unordered] [Alloc] in the direction of right.
void DnodeRelease(dnode* first);

#endif// !_INC_CPP
#endif// !_INC_DNODE
