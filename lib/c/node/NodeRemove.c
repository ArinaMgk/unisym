// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ19
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

void NodeHeapFreeSimple(pureptr_t inp) {
	Letvar(nod, Node*, inp);
	memf(nod->addr);
}

void NodeRemove(Node* nod, Node* left, void (*_node_freefunc)(pureptr_t ptxt))
{
	if (!nod) return;
	asserv(left)->next = nod->next;
	asserv(_node_freefunc)((pureptr_t)nod);
	memf(nod);
	// aflaga.fail = 0;
}

void NodesRelease(Node* nod, Node* left, _tofree_ft _node_freefunc)
{
	Node* crt = nod;
	if (!crt) return;
	Node* next;
	while (crt)
	{
		next = crt->next;
		// NodeRemove(crt, 0, _node_freefunc);
		{
			asserv(_node_freefunc)((pureptr_t)crt);
			memf(crt);
		}
		crt = next;
	}
	left->next = 0;
	//_node_crt = 0;
	//aflaga.zero = 1;
	//aflaga.one = 0;
	//aflaga.fail = 0;
}

void ChainDrop(chain_t* chain) {
	NodesRelease(chain->root_node, 0, chain->func_free);
}

