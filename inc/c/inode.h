// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ22
// AllAuthor: @dosconio
// ModuTitle: Identification Node
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

#ifndef _INC_INODE
#define _INC_INODE

#include "alice.h"

typedef struct IdenNode
{
	struct IdenNode* next;
	union { char* addr; void* offs; };// for order
	//
	void* data;
	size_t type;
	size_t property;
} inode;

#define INODE_READONLY 0x01
#define INODE_TYPEKEEP 0x02

// prop[2:Not-change-prevous]
inode* InodeUpdate(inode* inp, const char* iden, void* data, size_t typ, size_t prop, void(*freefunc_element)(void*));

// used name: InodeDelete ---> InodeRemove
void InodeDelete(inode* inp, const char* iden, void(*freefunc)(void*));

// InodeLocate ---> InodeIndex
inode* InodeLocate(inode* inp, const char* iden, inode** refleft);

// in the direction of right.
void InodesRelease(inode* first, void(*freefunc)(void*));

#endif
