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

#include "../../../inc/c/aldbg.h"
#include "../../../inc/c/ustring.h"
#include "../../../inc/c/tnode.h"

tnode* StrTokenAppend(tnode* any, const char* content, size_t contlen, size_t ttype, size_t row, size_t col)
{
	tnode* crt = any, * ret = 0;
	if (!contlen) return any;
	while (crt->next) crt = crt->next;
	memalloc(ret, sizeof(tnode));// Arinae style
	ret->left = crt;
	ret->next = 0;
	crt->next = ret;
	ret->addr = StrHeapN(content, contlen);// TODO. order&addr
	ret->len = ttype;//TODO. union{len, toktype}
	ret->row = row;
	ret->col = col;
	return ret;
}
