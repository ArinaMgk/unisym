// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ22
// AllAuthor: @ArinaMgk(till RFA03) @dosconio
// ModuTitle: Test for Double-directions Dnode
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
#define _CRT_SECURE_NO_WARNINGS
#define _LIB_STRING_HEAP
#include "../../../inc/c/ustring.h"
#include "../../../inc/c/dnode.h"
#include <stdio.h>

stduint malc_count;

static void DnodesPrint(dnode* first)
{
	dnode* next;
	while (first)
	{
		next = first->next;
		printf("<%" PRIdSTD ">\n", (intptr_t)first->addr);
		first = next;
	}
}

static void DnodesPrintString(dnode* first)
{
	dnode* next;
	while (first)
	{
		next = first->next;
		printf("<%" "s" ">\n", first->addr);
		first = next;
	}
}

char* StrHeap(const char* valit_str)
{
	size_t strlen = StrLength(valit_str);
	char* ret = zalc(strlen + 1);
	StrCopy(ret, valit_str);
	return ret;
}

int main()
{
	printf("\n%" PRIdSTD "\n", malc_count);

	dnode* nd = 0;
	void* old_memf_func = _dnode_freefunc;
	_dnode_freefunc = 0;
	// _node_order = _Dnode_Order_Insert;
	nd = DnodeAppend(nd, (void*)1, 0);
	DnodeInsert(nd, (void*)2, 0, 1);
	DnodeInsert(nd->next, (void*)4, 0, 1);
	DnodeInsert(nd->next, (void*)3, 0, 1);
	DnodesPrint(nd);
	DnodeRelease(nd);

	_dnode_freepass = 0;// do not do for (void*)1, (void*)2, (void*)3, (void*)4 ...

	puts("\n----\n");
	// _node_order = _Dnode_Order_Increase;
	{
		aflaga.autosort = 1;
		aflaga.direction = 0;
	}
	DnodeAppend(_dnode_first, (void*)1, 0);
	DnodeAppend(_dnode_first, (void*)2, 0);
	DnodeAppend(_dnode_first, (void*)4, 0);
	DnodeAppend(_dnode_first, (void*)3, 0);
	DnodesPrint(_dnode_first);
	DnodeRelease(_dnode_first);

	puts("\n----\n");
	// _node_order = _Dnode_Order_Decrease;
	{
		aflaga.autosort = 1;
		aflaga.direction = 1;
	}
	DnodeAppend(_dnode_first, (void*)1, 0);
	DnodeAppend(_dnode_first, (void*)2, 0);
	DnodeAppend(_dnode_first, (void*)4, 0);
	DnodeAppend(_dnode_first, (void*)3, 0);
	DnodesPrint(_dnode_first);
	DnodeRelease(_dnode_first);

	puts("\n----\n");
	// _node_order = _Dnode_Order_UserDefine;
	{
		aflaga.autosort = 0;
		_dnode_compare = (int(*)(void*, void*))strcmp;
	}
	_dnode_freefunc = old_memf_func;
	DnodeAppend(_dnode_first, StrHeap("123864"), 0);
	DnodeAppend(_dnode_first, StrHeap("923456"), 0);
	DnodeAppend(_dnode_first, StrHeap("323789"), 0);
	DnodeAppend(_dnode_first, StrHeap("523456"), 0);
	DnodesPrintString(_dnode_first);
	printf("the 323789 is %d span from first one plus one.\n", DnodeIndex(_dnode_first, "323789", 0, 1));
	printf("the 323788 is %d span from first one plus one. (not exist)\n", DnodeIndex(_dnode_first, "323788", 0, 1));
	printf("the count of the dnodes is %d\n", DnodeCount(_dnode_first));
	_dnode_freepass = 1;
	DnodeRelease(_dnode_first);

	puts("\n----\n");
	// _node_order = _Dnode_Order_Disable;
	{
		aflaga.autosort = 0;
		_dnode_compare = 0;
	}
	_dnode_freefunc = 0;
	DnodeAppend(_dnode_first, (void*)1, 0);
	DnodeAppend(_dnode_first, (void*)2, 0);
	DnodeAppend(_dnode_first, (void*)4, 0);
	DnodeAppend(_dnode_first, (void*)3, 0);
	DnodesPrint(_dnode_first);
	// _node_order = _Dnode_Order_Decrease;
	{
		aflaga.autosort = 1;
		aflaga.direction = 1;
	}
	DnodeSort(_dnode_first);
	DnodesPrint(_dnode_first);
	_dnode_freepass = 0;
	DnodeRelease(_dnode_first);

	puts("\n----\n");
	printf("\n%" PRIdSTD "\n", malc_count);
	return 0;
}


