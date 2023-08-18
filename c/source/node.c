// ASCII
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

// P-FA03 H-FR01
#include <stddef.h>
#include <stdlib.h>
#include "../node.h"
//
static inline int StrCompare(const char* a, const char* b)
{
	int tmp = 0;
	while (!(tmp = (*a - *b)) && *a++ && *b++); return tmp;
}
//
Node* NodeCreate(Node* previous, const char* data)
{
	if (previous)
	{
		previous = *(Node**)previous = (Node*)malloc(sizeof(Node));
	}
	else previous = (Node*)malloc(sizeof(Node));
	if (previous)
	{
		#ifdef _dbg
			malc_count++;
		#endif
		previous->data = (char*)data;
		previous->next = 0;
	}
	return previous;
}

Node* NodeCreateOrder(Node* previous, const char* data)
{
	if (previous)
	{
		for (; *(Node**)previous; previous = *(Node**)previous);
		previous = *(Node**)previous = (Node*)malloc(sizeof(Node));
	}
	else previous = (Node*)malloc(sizeof(Node));
	if (previous)
	{
		#ifdef _dbg
			malc_count++;
		#endif
		previous->data = (char*)data;
		previous->next = 0;
	}
	return previous;
}

size_t NodeIndex(Node* first, const char* cmp)
{
	size_t times = 0;
	Node* next;
	while (next = first)
		if ((times++, first = next->next, next->data) && !StrCompare(next->data, cmp))
			return times;
	return 0;
}

size_t NodeCount(Node* first)
{
	size_t ret = 0;
	while (first)
	{
		ret++;
		first = first->next;
	}
	return ret;
}

Node* NodeInsert(Node* first, const char* data)
{
	Node* tmp;
	if (first)
	{
		tmp = (Node*)malloc(sizeof(Node));
		#ifdef _dbg
			malc_count++;
		#endif
		tmp->data = (char*)data;
		tmp->next = first->next;
		first->next = tmp;
		return first;
	}
	else return NodeCreate(0, data);
	return 0;
}

void NodesRelease(Node* first)
{
	Node* next;
	while (next = first)
	{
		free((first = next->next, next));
		#ifdef _dbg
			malc_count--;
		#endif
	}
}

