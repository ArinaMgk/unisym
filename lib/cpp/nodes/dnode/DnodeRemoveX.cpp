// ASCII C/C++ TAB4 CRLF
// Docutitle: Node for Double-Direction Double-Field Linear Chain
// Codifiers: @dosconio: ~ 20240701
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
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

#include "../../../../inc/cpp/unisym"
#include "../../../../inc/c/dnode.h"

namespace uni {
	// impl Array for 
	// [no-use] LocateLeft

	bool    Dchain::Remove(stduint idx, stduint times) {
		if (!times) return true;
		// if (!root_node) return false;
		Dnode* crt = (Dnode*)Locate(idx);
		if (!crt) return false;
		Dnode* const last = crt->left;
		Dnode* const next = (Dnode*)Locate(idx + times);
		for0(i, times) {
			Dnode* const nex = crt->next;
			DnodeRemove(crt, func_free);
			crt = nex;
		}
		asserv(last)->next = next;
		DnodeChainAdapt(last ? root_node : next, next ? last_node : last, -(stdint)times);
		return true;
	}

	Dnode* Dchain::Remove(Dnode* nod) {
		Dnode* crt = Root(), * nex, * las = 0;
		if (!crt) return nullptr;
		do {
			nex = crt->next;
			if (crt != nod) continue;
			DnodeRemove(crt, func_free);
			DnodeChainAdapt(crt == root_node ? nex : root_node, crt == last_node ? las : last_node, -1);
			break;
		} while (AssignParallel(las, crt, nex));
		return nex;
	}

	Dnode* Dchain::Remove(pureptr_t content) {
		Dnode* crt = Root(), * nex, * las = 0;
		if (!crt) return nullptr;
		do {
			nex = crt->next;
			if (crt->offs != content) continue;
			DnodeRemove(crt, func_free);
			DnodeChainAdapt(crt == root_node ? nex : root_node, crt == last_node ? las : last_node, -1);
			break;
		} while (AssignParallel(las, crt, nex));
		return nex;
	}
}




