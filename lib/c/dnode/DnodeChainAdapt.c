// ASCII C99 TAB4 CRLF
// AllAuthor: @dosconio
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

enum { ON_LEFT = 0, ON_RIGHT = 1 };

void   DnodeChainAdapt(dchain_t* chn, Dnode* root, Dnode* last, stdint count_dif)
{
	chn->node_count += count_dif;
	chn->root_node = root;
	chn->last_node = last;
	//[Fast Table except root/last node]
	// assume 35 items, consider 33 items, 33 / 2 + 1 = 17;
	// assume 3 items ... <=> 3 / 2 = 1;
	#if 0
	if (node_count < 2 + 1) // root, last, and 
		fastab.midl_node = NULL;
	else
		fastab.midl_node = LocateNode(node_count >> 1);
	#endif
}
