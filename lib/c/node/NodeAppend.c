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

#define on_decresing_order (aflaga.direction)
#define on_increasing_order (!aflaga.direction)

node* NodeAppend(node* first, void* addr)
{
	node* tmp = zalcof(node);
	node* last = first;
	tmp->offs = addr;
	_node_first = first;

	aflaga.fail = 0;
	aflaga.zero = 0;
	aflaga.one = 0;
	// if (_node_order != _Node_Order_Insert)
	// {
	if (!first)
	{
		aflaga.one = 1;
		return _node_first = tmp;
	}
	if (aflaga.autosort)
	{
		if ((addr <= first->offs) ^ on_decresing_order)
		{
			// insert left
			tmp->next = first;
			return _node_first = tmp;
		}
		while (first->next && ((first->next->offs < addr) ^ on_decresing_order))
		{
			last = first;
			first = first->next;
		}
		if ((first->offs < addr) ^ on_decresing_order)
			last = first;
		// insert right
	}
	else if (_node_compare)
	{
		if (_node_compare(addr, first->offs) <= 0)
		{
			// insert left
			tmp->next = first;
			return _node_first = tmp;
		}
		while (first->next && _node_compare(first->next->offs, addr) < 0)
		{
			last = first;
			first = first->next;
		}
		if (_node_compare(first->offs, addr) < 0)
			last = first;
		// insert right
	}
	else while (first->next)
	{
		last = first;
		first = first->next;
	}

	if (first)// _node_order == _Node_Order_Insert and other conditions inserting right
	{
		// insert right
		tmp->next = first->next;
		last->next = tmp;
	}
	return tmp;
}
