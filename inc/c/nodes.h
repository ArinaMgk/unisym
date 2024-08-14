// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ21
// AllAuthor: @ArinaMgk(till RFA03) @dosconio
// ModuTitle: Nodes Family
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

// node-family compatible chain{anode[a] > nnode[8] > ... :> dnode[4] > node[2]}
//{TODO} pnode for C++ template (pnode<struct xxx{...}>)

// ":>" means not be compatible with smaller, but the next bigger level maybe compatible. E.g. ... :> dnode[4], t contains d, but i not.

// autosort do not contain the case of user-defined compare function
// autosort affect by the value of `addr`
// priority: autosort > usersort > unsort

#ifndef _INC_NODES
#define _INC_NODES

#include "node.h"
#include "dnode.h"
#include "nnode.h"
#include "anode.h"

//{TODO & UNREGISTERED} Conversion Function (direct copy address of pointer) 
/*
inline static dnode* NnodeToDnode(nnode* inp)
{
	if (sizeof(nnode) < sizeof(dnode)) return 0;
	nnode tmpn, *next, *crt = inp;
	dnode tmpd;
	//while (crt)//---> [4]
	if (crt)
	{
		next = crt->next;
		tmpn = *crt;
		tmpd.addr = crt->addr;
		tmpd.left = (void*)crt->left;
		tmpd.next = (void*)crt->next;
		tmpd.type = crt->type;
		MemCopyN(crt, &tmpd, sizeof tmpd);
		crt = next;
	}
	return (void*)inp;// null ---> null
}

inline static tnode* NnodeToTnode(nnode* inp)
{
	if (sizeof(nnode) < sizeof(dnode)) return 0;
	nnode tmpn, * next, * crt = inp;
	tnode tmpd;
	//while (crt)//---> [6]
	if (crt)
	{
		next = crt->next;
		tmpn = *crt;
		tmpd.addr = crt->addr;
		tmpd.left = (void*)crt->left;
		tmpd.next = (void*)crt->next;
		tmpd.type = crt->type;
		tmpd.row = crt->row;
		tmpd.col = crt->col;
		MemCopyN(crt, &tmpd, sizeof tmpd);
		crt = next;
	}
	return (void*)inp;
}

inline static void NnodeForeach(nnode* start, nnode* parent, void(*func)(nnode* self, nnode* parent))
{
	nnode* crt = start;
	while (crt)
	{
		// do with children firstly
		if (crt->subf) NnodeForeach(crt->subf, crt, func);
		func(crt, parent);
		crt = crt->next;
	}
}

inline static void TnodeToNnode(nnode* des, const tnode* src)
{
	des->col = src->col;
	des->row = src->row;
	des->addr = src->addr;
	des->type = src->type;
}

*/
#endif
