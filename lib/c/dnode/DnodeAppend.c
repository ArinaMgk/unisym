// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ21
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

#define on_decresing_order (aflaga.direction)
#define on_increasing_order (!aflaga.direction)

dnode* DnodeAppend(dnode* any, void* addr, size_t typlen)
{
	dnode* tmp = zalcof(dnode);
	dnode* crt = any;
	tmp->offs = addr;
	tmp->type = typlen;

	aflaga.fail = 0;
	aflaga.zero = 0;
	aflaga.one = !any;
	if (!any)
		return _dnode_first = tmp;
	///int _localbit_dec = on_decresing_order;
lup:// Split into two directions will be faster
	if (aflaga.autosort)
	{
		if ((addr < crt->offs) ^ on_decresing_order)
		{
			if(crt->left && ((addr < crt->left->offs) ^ on_decresing_order))
			{
				crt = !on_decresing_order ? crt->left : crt->next;
				goto lup;
			}
			else // create in the left
			{
				// (A) crt (C)
				// (A-) -tmp- -crt (C)
				tmp->left = crt->left;
				tmp->next = crt;
				if (crt->left) crt->left->next = tmp;
				crt->left = tmp;
				if (!tmp->left) _dnode_first = tmp;
			}
		}
		else // (addr >= any->offs) ^ on_decresing_order
		{
			if(crt->next && ((addr >= crt->next->offs) ^ on_decresing_order))
			{
				crt = !on_decresing_order ? crt->next : crt->left;
				goto lup;
			}
			else // create in the right
			{
				// (A) crt (C)
				// (A) crt- -tmp- (-C)
				tmp->left = crt;
				tmp->next = crt->next;
				if (crt->next) crt->next->left = tmp;
				crt->next = tmp;
			}
		}
	}
	else if (_dnode_compare)
	{
		if (_dnode_compare(addr, crt->offs) <= 0)
		{
			if(crt->left && _dnode_compare(addr, crt->left->offs) < 0)
			{
				crt = crt->left;
				goto lup;
			}
			else // create in the left
			{
				// (A) crt (C)
				// (A-) -tmp- -crt (C)
				tmp->left = crt->left;
				tmp->next = crt;
				if (crt->left) crt->left->next = tmp;
				crt->left = tmp;
				if (!tmp->left) _dnode_first = tmp;
			}
		}
		else // (addr > any->offs)
		{
			if(crt->next && _dnode_compare(addr, crt->next->offs) > 0)
			{
				crt = crt->next;
				goto lup;
			}
			else // create in the right
			{
				// (A) crt (C)
				// (A) crt- -tmp- (-C)
				tmp->left = crt;
				tmp->next = crt->next;
				if (crt->next) crt->next->left = tmp;
				crt->next = tmp;
			}
		}
	}
	else
	{
		while (crt->next)
			crt = crt->next;
		// insert right
		tmp->left = crt;
		crt->next = tmp;
	}

	return tmp;
}
