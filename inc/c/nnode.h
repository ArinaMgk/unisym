// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ22
// AllAuthor: @dosconio
// ModuTitle: Nested Node
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

#include "alice.h"

typedef struct NestNode
{
	// { tode }
	struct NestNode* next;
	union { char* addr; void* offs; };
	struct NestNode* left;
	size_t type;// from `class` for C++ keyword compatibility
	size_t row, col;
	//
	struct NestNode* subf;// sub-first item
	union { void* bind; size_t flag; struct NestNode* pare; };
} nnode;// {comb with nnode}

// param:direction [0:L 1:R]#cancelled[2:SubHead 3:SubTail]
nnode* NnodeInsert(nnode* nod, int direction, nnode* parent);

// Set a part of nnode as the sub of a nnode. If subtail is null, this is for all the right part of the nnode string. If only one item, keep subhead and subtail same.
nnode* NnodeBlock(nnode* nod, nnode* subhead, nnode* subtail, nnode* parent);

// Free for self and its addr.
void NnodeReleaseTofreeDefault(void* inp);

// If freefunc is not null, free for memory will be defined by user, or just free the node block. In the direction of right.
void NnodeRelease(nnode* nod, nnode* parent, void(*freefunc)(void*));

// If freefunc is not null, free for memory will be defined by user, or just free the node block. In the direction of right.
void NnodesRelease(nnode* nod, nnode* parent, void(*freefunc)(void*));

#endif // !_INC_NNODE
