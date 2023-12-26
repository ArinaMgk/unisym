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

#include "../../../inc/c/aldbg.h"
#include "../../../inc/c/dnode.h"

#define on_decresing_order (aflaga.direction)
#define on_increasing_order (!aflaga.direction)

dnode* DnodeInsert(dnode* nod, void* addr, size_t typlen, int direction)
{
	dnode* tmp = zalcof(dnode);
	dnode* crt = nod;
	tmp->addr = addr;
	tmp->type = typlen;

	aflaga.fail = 0;
	aflaga.zero = 0;
	aflaga.one = !nod;
	if (!nod)
		return _dnode_first = tmp;

	switch (direction)
	{
	case -2:
		// insert head
		tmp->next = DnodeRewind(nod);
		tmp->next->left = tmp;
		_dnode_first = tmp;
		break;
	case -1:
		// insert left
		// (A) crt (C)
		// (A-) -tmp- -crt (C)
		tmp->left = crt->left;
		tmp->next = crt;
		if (crt->left) crt->left->next = tmp;
		crt->left = tmp;
		if (!tmp->left) _dnode_first = tmp;
		break;
	case 1:
		// insert right
		// (A) crt (C)
		// (A) crt- -tmp- (-C)
		tmp->left = crt;
		tmp->next = crt->next;
		if (crt->next) crt->next->left = tmp;
		crt->next = tmp;
		break;
	case 2:
		// insert tail
		while (crt->next) crt = crt->next;
		tmp->left = crt;
		tmp->left->next = tmp;
		break;
	default:
		aflaga.fail = 1;
		memf(tmp);
		return 0;
	}
	return tmp;
}
