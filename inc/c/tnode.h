// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ22
// AllAuthor: @dosconio
// ModuTitle: Token Node
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

// base of ustring::token system

#ifndef _INC_TNODE
#define _INC_TNODE

#include "alice.h"
#include "ustring.h"

typedef struct TokenNode
{
	// {dnode} 
	struct TokenNode* next;
	union { char* addr; void* offs; };
	struct TokenNode* left;
	union { size_t len; toktype type; };
	size_t row, col;
} tode, tnode;// recommand using tnode. measures pointer[6]

#define _TNODE_COMMENT '#'
#define _TNODE_DIRECTIVE '%'

//
tnode* StrTokenAppend(tnode* any, const char* content, size_t contlen, size_t ttype, size_t row, size_t col);

// ...
#define TnodeLoad StrTokenAll
tnode* StrTokenAll(int (*getnext)(void), void (*seekback)(ptrdiff_t chars), char* buffer);

// {TODO} Merged into StrTokenReleases
void StrTokenClearAll(tnode* tstr);

// Free for self and its addr.
void TnodeReleaseTofreeDefault(void* inp);

// freefunc should memf the parameter-pointed object besides its resources.
void TnodesReleases(tnode* nod, void(*freefunc)(void*));

//
void StrTokenThrow(tnode* one);// a b c --> a c

//
inline static tnode* StrTokenBind(tnode* left, tnode* mid, tnode* right)
{
	if (left) left->next = mid;
	if (mid) mid->next = right;
	if (right) right->left = mid;
	if (mid) mid->left = left;
	return mid;
}

#define StrTokenPrint(first)\
	printf("Token: [R %llu,C %llu][%s] %s\n", first->row, first->col,\
		((const char* []){"END", "ANY", "STR", "CMT", "DIR", "NUM", "SYM", "IDN", "SPC", "ELS"})\
		[first->type], first->type == tok_space ? "" : first->addr);

// For the string of the Tode.
#define StrTokenPrintAll(first)\
	do StrTokenPrint(first);\
	while (first = first->next);


#endif
