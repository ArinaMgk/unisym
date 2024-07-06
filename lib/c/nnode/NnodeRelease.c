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

Nnode* NnodeRelease(nnode* nod, _tofree_ft freefunc)
{
	Nnode* res = nod->next;
	if (Nnode_isEldest(nod)) nod->pare->subf = nod->next;
	if (nod->subf) NnodesRelease(nod->subf, freefunc);
	if (nod->left) nod->left->next = nod->next;
	if (nod->next) nod->next->left = nod->left;
	if (freefunc) freefunc(nod); else memf(nod);
	return res;
}
