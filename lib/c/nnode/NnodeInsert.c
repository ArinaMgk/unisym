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


#include "../../../inc/c/nnode.h"

Nnode* NnodeInsert(Nnode* nod, pureptr_t offs, stduint typlen, stduint extn_field, int direction_right)
{
	Letvar(tmp, Nnode*, zalc(sizeof(Nnode) + extn_field));
	tmp->type = typlen;
	tmp->offs = offs;
	if (!nod) return tmp;
	if (direction_right == 0) // left
	{
		if (Nnode_isEldest(nod)) {
			if (tmp->left = nod->left)
				nod->pare->subf = tmp;
		}
		else {
			if (tmp->left = nod->left)
				nod->left->next = tmp;
		}
		nod->left = tmp;
		AssignParallel(tmp->next, nod->next, tmp);
	}
	else // right
	{
		if (tmp->next = nod->next) tmp->next->left = tmp;
		(tmp->left = nod)->next = tmp;
	}
	return tmp;
}
