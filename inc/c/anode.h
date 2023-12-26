// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ22
// AllAuthor: @ArinaMgk
// ModuTitle: Arn Old-Style Node
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

#ifndef _INC_XNODE
#define _INC_XNODE

#include "alice.h"

// Difference: ANode has address of its parent.
typedef struct ArnOldStyleNode
{
	// { nnode }
	struct ArnOldStyleNode* next;
	union { char* addr; void* offs; };// for ASCIZ string normally
	struct ArnOldStyleNode* left;
	size_t type;
	size_t row, col;
	struct ArnOldStyleNode* subf;
	struct ArnOldStyleNode* pare;// parent
	//
	struct ArnOldStyleNode* alias;
	void* data;
} anode;// recommand using nnode. measures pointer[a]

// rename: RFZ22 cancel anode and give name to xnode.

#endif
