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

// [Unordered] [Alloc]
void DnodeRemove(dnode* some)
{
	if (!some) return;
	if (!some->left) _dnode_first = some->next;
	aflaga.zero = !_dnode_first;
	aflaga.one = _dnode_first && !_dnode_first->next;

	if (some->left) some->left->next = some->next;
	if (some->next) some->next->left = some->left;
	if (_dnode_freefunc)
		_dnode_freefunc(_dnode_freepass ? (void*)some : some->offs);
	else memf(some);
	aflaga.fail = 0;
}
