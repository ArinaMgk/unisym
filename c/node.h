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

#ifndef ModNode
#define ModNode
/*
	single-directions simple node
	Node- for a d-d node item
	Nodes- for a d-d node chain
*/
// using stddef.h
typedef struct Node
{
	struct Node* next;
	char* data;
} Node;

#ifdef _dbg
	extern size_t malc_count;
#endif

Node* NodeCreate(Node* previous, const char* data);
Node* NodeCreateOrder(Node* previous, const char* data);
size_t NodeIndex(Node* first, const char* cmp);
size_t NodeCount(Node* first);
Node* NodeInsert(Node* obj, const char* data);
void NodesRelease(Node* first);

#endif