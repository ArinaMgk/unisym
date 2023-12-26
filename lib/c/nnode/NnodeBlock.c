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

#include "../../../inc/c/aldbg.h"
#include "../../../inc/c/nnode.h"

nnode* NnodeBlock(nnode* nod, nnode* subhead, nnode* subtail, nnode* parent)
{
	if (subhead->left == subtail)// for empty parens and parend "(" ")"
		return nod;
#ifdef _LIB_DEBUG_CHECK_MORE
	{
		nnode* crt = subhead;
		while (crt)
		{
			if(crt == subtail) break;
			crt = crt->next;
		}
		if (crt != subtail) erro("subtail not be in the right of subhead.")
	}
#endif
	// Above: "(" no right, subhead zo ")"; ")" no left, subtail zo "("
	nnode* subleft = subhead->left, * subright = subtail ? subtail->next : 0;
	nod->subf = subhead;
	// [nod] [] ... [sub1] [sub2] ... []
	if (parent && parent->subf == subhead) parent->subf = nod;
	if (subleft) subleft->next = subright;
	if (subright) subright->left = subleft;
	subhead->left = subtail->next = 0;
	return nod;
}
