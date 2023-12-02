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

node* NodeAppend(node* first, void* addr)
{
	node* tmp = zalcof(node);
	node* last = first;
	tmp->addr = addr;
	_node_first = first;
	if (_node_order != _Node_Order_Insert)
	{
		if (!first) return _node_first = tmp;
		if (_node_order <= _Node_Order_Decrease)
		{
			if ((addr <= first->addr) ^ (_node_order == _Node_Order_Decrease))
			{
				// insert left
				tmp->next = first;
				return _node_first = tmp;
			}
			while (first->next && ((first->next->addr < addr) ^ (_node_order == _Node_Order_Decrease)))
			{
				last = first;
				first = first->next;
			}
			if ((first->addr < addr) ^ (_node_order == _Node_Order_Decrease))
				last = first;
			// insert right
		}
		else if (_node_order == _Node_Order_UserDefine && _node_compare)
		{
			if (_node_compare(addr, first->addr) <= 0)
			{
				// insert left
				tmp->next = first;
				return _node_first = tmp;
			}
			while (first->next && _node_compare(first->next->addr, addr) < 0)
			{
				last = first;
				first = first->next;
			}
			if (_node_compare(first->addr, addr) < 0)
				last = first;
			// insert right
		}
		else if(_node_order == _Node_Order_Disable)
		{
			while (first->next)
			{
				last = first;
				first = first->next;
			}
			last = first;
		}
		else return 0;/// erro()
	}

	if (first)// _node_order == _Node_Order_Insert and other conditions inserting right
	{
		// insert right
		tmp->next = first->next;
		last->next = tmp;
	}
	return tmp;
}
