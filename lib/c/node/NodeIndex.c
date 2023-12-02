// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[ ]
// LastCheck: RFZ02
// AllAuthor: @ArinaMgk(till RFA03) @dosconio
// ModuTitle: Simple Node
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

#include "../../../inc/c/node.h"
#include "../../../inc/c/aldbg.h"

static int isequal(void* addr0, void* addr1)
{
	return (int)((ptrdiff_t)addr0 - (ptrdiff_t)addr1);
}

size_t NodeIndex(node* first, void* addr)
{
	size_t times = 0;
	node* next;
	while (next = first)
		if ((times++, first = next->next, next->addr) && !(_node_compare ? _node_compare : isequal)(next->addr, addr))
			return times;
	return 0;
}

