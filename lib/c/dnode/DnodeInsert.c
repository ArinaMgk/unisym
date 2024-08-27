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


#include "../../../inc/c/dnode.h"

#define on_decresing_order (aflaga.direction)
#define on_increasing_order (!aflaga.direction)

Dnode* DnodeInsert(Dnode* nod, pureptr_t offs, size_t typlen, stduint extn_field, int direction_right)
{
	Letvar(tmp, Dnode*, zalc(sizeof(Dnode) + extn_field));
	if (!tmp) return 0;
	tmp->offs = offs;
	tmp->type = typlen;
	if (direction_right) {
		if (!(tmp->left = nod)) return tmp;
		asserv(nod->next)->left = tmp;
		return AssignParallel(tmp->next, nod->next, tmp);
	}
	else {
		if (!(tmp->next = nod)) return tmp;
		asserv(nod->left)->next = tmp;
		return AssignParallel(tmp->left, nod->left, tmp);
	}
}
