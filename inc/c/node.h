// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ02
// AllAuthor: @ArinaMgk(till RFA03) @dosconio
// ModuTitle: Simple Node
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

// AFLAGA {ASF, DF, FF, ZF, ONF}
// - ASF : Auto sort the node by the DF
// - DF  : in increasing or decreasing order
// - FF  : something wrong raised
// - ZF  : zero flag, if the node is empty
// - ONF : order flag, if the node has and only has one node

#ifndef _INC_NODE
#define _INC_NODE

#include "alice.h"

#ifdef _INC_CPP
class Node {
public:
	Node* next;
	const void* offs;
};
#else
typedef struct node {
	struct node* next;
	union { const char* addr; const void* offs; };
} node; // measures stdint[2]
#endif

// extern enum _Node_Order
// {
//  _Node_Order_Increase,   //  ASF & !DF [default]
//  _Node_Order_Decrease,   //  ASF &  DF
//  _Node_Order_UserDefine, // !ASF &  _node_compare!=0
//  _Node_Order_Disable,    // !ASF &  _node_compare==0
//  _Node_Order_Insert      // cancelled
// }
// _node_order;

#ifndef _INC_CPP

// default null
// return 0 for equal, 1 for greater, -1 for less
extern int (*_node_compare)(void* addr0, void* addr1);

// default provided by `memf()` for `addr`, not the `node` itself
extern void(*_node_freefunc)(void*);

// mark the head of the chain after last `NodeAppend()` , will be reset to zero after `NodeRelease()` 
extern node* _node_first;

// ---- ---- ---- ---- Structure ---- ---- ---- ---- 

// Whether the order is enabled or disabled depends on `_node_order`.
// Return the appended node.
node* NodeAppend(node* first, void* addr);

// 
node* NodeInsert(node* first, void* addr);

// ---- ---- ---- ---- Sort and Seek ---- ---- ---- ---- 

// Sort the chain by `addr` in increasing or decreasing order.
node* NodeSort(node* first);

// Return the distance plus one, or 0 for not found.
size_t NodeIndex(const node* first, void* addr);

// Return the counts of the node string.
size_t NodeCount(const node* first);

// ---- ---- ---- ---- Destruction ---- ---- ---- ---- 

// Remove a node from the chain.
void NodeRemove(node* nod, node* left);

// If `tofree` is zero, the `addr` of nodes in the chain will be released by single `memf()` . If you do not want to release the `addr` or call `freefunc`, set `addr` to zero.
void NodeRelease(node* first);

#endif
#endif
