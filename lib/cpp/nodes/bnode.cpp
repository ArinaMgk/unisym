// ASCII CPP TAB4 CRLF
// Docutitle: Binary Tree Node Chain, not Token-Node-Chain!
// Datecheck: 20240416 ~ 20240421
// Developer: @dosconio @ UNISYM
// Attribute: [Allocate]
// Reference: None
// Dependens: None
// Copyright: UNISYM, under Apache License 2.0
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

#include "../../../inc/cpp/bnode"

namespace uni {
	void BnodeChain::Traversal(TraversalOrder order, bool (*procfunc)(Bnode*), Bnode* starter) {
		nulrecurs(starter, Root(), );
		switch (order) {
		case Preorder:
			if (procfunc) if (!procfunc(starter)) break;
			Traversal(order, procfunc, starter->left);
			Traversal(order, procfunc, starter->next);
			break;
		case Inorder:
			Traversal(order, procfunc, starter->left);
			if (procfunc) if (!procfunc(starter)) break;
			Traversal(order, procfunc, starter->next);
			break;
		case Postorder:
			Traversal(order, procfunc, starter->left);
			Traversal(order, procfunc, starter->next);
			if (procfunc) if (!procfunc(starter)) break;
			break;
		default:
			break;
		}
	}

	stduint BnodeChain::Leaves(Bnode* starter) {
		nulrecurs(starter, Root(), 0);
		if (starter->left == nullptr && starter->next == nullptr) return 1;
		stduint leftLeaves = Leaves(starter->left);
		stduint rightLeaves = Leaves(starter->next);
		return leftLeaves + rightLeaves;
	}

	stduint BnodeChain::Depth(Bnode* starter) {
		nulrecurs(starter, Root(), 0);
		stduint leftDepth = Depth(starter->left);
		stduint rightDepth = Depth(starter->next);
		return maxof(leftDepth, rightDepth) + 1;
	}
}

