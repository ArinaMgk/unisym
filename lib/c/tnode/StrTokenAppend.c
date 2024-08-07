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


#include "../../../inc/c/ustring.h"
#include "../../../inc/c/dnode.h"

Dnode* StrTokenAppend(tchain_t* chn, const char* content, size_t contlen, size_t ttype, size_t row, size_t col)
{
	if (!contlen && ttype != tok_string) return 0;
	chn->last_node = DnodeInsert(chn->last_node, (pureptr_t)StrHeapN(content, contlen), ttype, sizeof(TnodeField), 1/*ON_RIGHT*/);
	TnodeField* tfield = TnodeGetExtnField(*chn->last_node);
	tfield->row = row;
	tfield->col = col;
	if (!chn->root_node) return chn->root_node = chn->last_node;
	return chn->last_node;
}
