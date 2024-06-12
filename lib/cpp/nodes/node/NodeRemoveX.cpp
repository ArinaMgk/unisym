// ASCII C++ TAB4 CRLF
// Attribute: Allocate(Need)
// LastCheck: unchecked for C++ version
// AllAuthor: @dosconio
// ModuTitle: Node for C++
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

#include "../../../../inc/cpp/unisym"
#include "../../../../inc/c/node.h"

namespace uni {
	// impl Array for 
	// [no-use] LocateLeft

	bool    Chain::Remove(stduint idx, stduint times) {
		if (!times) return true;
		// if (!root_node) return false;
		Node* crt = (Node*)Locate(idx);
		if (!crt) return false;
		Node* const last = getLeft(crt, false);
		Node* const next = (Node*)Locate(idx + times);
		for0 (i, times) {
			Node* const nex = crt->next;
			NodeRemove(crt, nullptr, func_free);
			crt = nex;
		}
		asserv(last)->next = next;
		NodeChainAdapt(last ? root_node : next, next ? last_node : last, -(stdint)times);
		return true;
	}

	bool Chain::Remove(Node* nod) {
		Node* crt = Root(), * nex, * las = 0;
		if (!crt) return false;
		do {
			nex = crt->next;
			if (crt != nod) continue;
			NodeRemove(crt, las, func_free);
			NodeChainAdapt(crt == root_node ? nex : root_node, crt == last_node ? las : last_node, -1);
			break;
		} while (AssignParallel(las, crt, nex));
		return true;
	}

	bool Chain::Remove(pureptr_t content) {
		Node* crt = Root(), * nex, * las = 0;
		if (!crt) return false;
		do {
			nex = crt->next;
			if (crt->offs != content) continue;
			NodeRemove(crt, las, func_free);
			NodeChainAdapt(crt == root_node ? nex : root_node, crt == last_node ? las : last_node, -1);
			break;
		} while (AssignParallel(las, crt, nex));
		return true;
	}
}
