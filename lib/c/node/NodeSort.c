// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
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

node* NodeSort(node* first)
{
	struct _aflag_t af = aflaga;
	_node_first = 0;
	if (!first || (!aflaga.autosort && _node_compare)) return 0;
	node* next, * crt = first;
	while (crt)
	{
		next = crt->next;
		NodeAppend(_node_first, crt->addr);
		crt = next;
	}
	crt = first;
	while (crt)
	{
		next = crt->next;
		memf(crt);
		crt = next;
	}
	aflaga = af;
	return _node_first;
}
