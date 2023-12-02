// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ02
// AllAuthor: @ArinaMgk(till RFA03) @dosconio
// ModuTitle: Test for Simple Node
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

#include<stdio.h>
#include<string.h>
#include<inttypes.h>
#include"../../../inc/c/node.h"
#define _LIB_STRING_HEAP
#include"../../../inc/c/ustring.h"

size_t malc_count;

static void NodesPrint(node* first)
{
	node* next;
	while (first)
	{
		next = first->next;
		printf("<%" PRIdPTR ">\n", (intptr_t)first->addr);
		first = next;
	}
}

static void NodesPrintString(node* first)
{
	node* next;
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

void main()
{
	printf("\n%" PRIdPTR "\n", malc_count);

	node* nd = 0;
	void* old_memf_func = _node_freefunc;
	_node_freefunc = 0;
	_node_order = _Node_Order_Insert;
	nd = NodeAppend(nd, (void*)1);
	NodeAppend(nd, (void*)2);
	NodeAppend(nd->next, (void*)4);
	NodeAppend(nd->next, (void*)3);
	NodesPrint(nd);
	NodeRelease(nd);

	puts("\n----\n");
	_node_order = _Node_Order_Increase;
	NodeAppend(_node_first, (void*)1);
	NodeAppend(_node_first, (void*)2);
	NodeAppend(_node_first, (void*)4);
	NodeAppend(_node_first, (void*)3);
	NodesPrint(_node_first);
	NodeRelease(_node_first);

	puts("\n----\n");
	_node_order = _Node_Order_Decrease;
	NodeAppend(_node_first, (void*)1);
	NodeAppend(_node_first, (void*)2);
	NodeAppend(_node_first, (void*)4);
	NodeAppend(_node_first, (void*)3);
	NodesPrint(_node_first);
	NodeRelease(_node_first);

	puts("\n----\n");
	_node_order = _Node_Order_UserDefine;
	_node_compare = (int(*)(void*, void*))strcmp;
	_node_freefunc = old_memf_func;
	NodeAppend(_node_first, StrHeap("123864"));
	NodeAppend(_node_first, StrHeap("923456"));
	NodeAppend(_node_first, StrHeap("323789"));
	NodeAppend(_node_first, StrHeap("523456"));
	NodesPrintString(_node_first);
	printf("the 323789 is %d span from first one plus one.\n", NodeIndex(_node_first, "323789"));
	printf("the 323788 is %d span from first one plus one. (not exist)\n", NodeIndex(_node_first, "323788"));
	printf("the count of the nodes is %d\n", NodeCount(_node_first));
	NodeRelease(_node_first);

	puts("\n----\n");
	_node_order = _Node_Order_Disable;
	_node_compare = 0;
	_node_freefunc = 0;
	NodeAppend(_node_first, (void*)1);
	NodeAppend(_node_first, (void*)2);
	NodeAppend(_node_first, (void*)4);
	NodeAppend(_node_first, (void*)3);
	NodesPrint(_node_first);
	_node_order = _Node_Order_Decrease;
	NodeSort(_node_first);
	NodesPrint(_node_first);
	NodeRelease(_node_first);

	puts("\n----\n");
	printf("\n%" PRIdPTR "\n", malc_count);
}
