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

#ifndef _INC_NODE
#define _INC_NODE

#include "alice.h"

typedef struct node
{
	struct node* next;
	void* addr;
} node;// measures stdint[2]

extern enum _Node_Order
{
	_Node_Order_Increase, // default
	_Node_Order_Decrease,
	_Node_Order_UserDefine, // increasing order
	_Node_Order_Disable,
	_Node_Order_Insert, // for `NodeAppend()`, insert a node between the node and its next item, the `first` may not be the first node actually
}
_node_order;

// default null
// return 0 for equal, 1 for greater, -1 for less
extern int (*_node_compare)(void* addr0, void* addr1);

// default provided by `memf()`
extern void(*_node_freefunc)(void*);

// mark the head of the chain after last `NodeAppend()` , will be reset to zero after `NodeRelease()` 
extern node* _node_first;

// ---- ---- ---- ---- Structure ---- ---- ---- ---- 

// Whether the order is enabled or disabled depends on `_node_order`.
// Return the appended node.
node* NodeAppend(node* first, void* addr);

// ---- ---- ---- ---- Sort and Seek ---- ---- ---- ---- 

// Sort the chain by `addr` in increasing or decreasing order.
node* NodeSort(node* first);

// Return the distance plus one, or 0 for not found.
size_t NodeIndex(node* first, void* addr);

// Return the counts of the node string.
size_t NodeCount(node* first);

// ---- ---- ---- ---- Destruction ---- ---- ---- ---- 

// If `tofree` is zero, the `addr` of nodes in the chain will be released by single `memf()` . If you do not want to release the `addr` or call `freefunc`, set `addr` to zero.
void NodeRelease(node* first);

// ---- ---- ---- ---- TO DO ---- ---- ---- ----
//{TODO} INLINE DATA
//{TODO} Replace _Node_Order into AFLAGA {ASF, DF, FF, ZF, ONF}
// - ASF : Auto sort the node by the DF
// - DF  : in increasing or decreasing order
// - FF  : something wrong raised
// - ZF  : zero flag, if the node is empty
// - ONF : order flag, if the node has and only has one node
//{TODO} NodeInsert()


#endif
