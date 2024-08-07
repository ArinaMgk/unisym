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

// [Alloc]
void DnodeRemove(Dnode* nod, _tofree_ft _dnode_freefunc)
{
	if (!nod) return;
	asserv(nod->left)->next = nod->next;
	asserv(nod->next)->left = nod->left;
	asserv(_dnode_freefunc)((pureptr_t)nod);
	memf(nod);
	// aflaga.fail = 0;
}
