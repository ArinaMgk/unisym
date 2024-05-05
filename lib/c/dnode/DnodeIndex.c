// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant 
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

dnode* DnodeIndex(const dnode* any, void* addr, stdint* ref_span, int direction)
{
	dnode* tmp = (dnode*)any;// formal, and will not be changed really
	stdint i = 0;
	aflaga.fail = 0;

	if (!any)
		return 0;

	switch (direction)
	{
	case -2:
		// from head
		while (tmp->left)
		{
			tmp = tmp->left;
			i--;
		}
		while (tmp)
		{
			if (tmp->addr == addr)
				break;
			tmp = tmp->next;
			i++;
		}
		aflaga.fail = 1;
		break;
	case -1:
		// from left to head
		while (tmp)
		{
			if (tmp->addr == addr)
				break;
			tmp = tmp->left;
			i--;
		}
		aflaga.fail = 1;
		break;
	case 1:
		// from right to tail
		while (tmp)
		{
			if (tmp->addr == addr)
				break;
			tmp = tmp->next;
			i++;
		}
		aflaga.fail = 1;
		break;
	case 2:
		// from tail
		while (tmp->next)
		{
			tmp = tmp->next;
			i++;
		}
		while (tmp)
		{
			if (tmp->addr == addr)
				break;
			tmp = tmp->left;
			i--;
		}
		aflaga.fail = 1;
		break;
	}
	if (ref_span) *ref_span = i;
	return tmp;
}
