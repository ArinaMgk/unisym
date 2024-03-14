// ASCII C++-20 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: 20240312
// AllAuthor: @dosconio
// ModuTitle: Nested Node and Syntax Parser
// Depend-on: Nnode.cpp Tnode.cpp
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

#include "../../../../inc/cpp/cinc"
#include "../../../../inc/c/aldbg.h"
#include "../../../../inc/cpp/cinc"
#include "../../../../inc/cpp/nnode"
#include <new>

namespace uni {
	NestedParseUnit::NestedParseUnit(TnodeChain& nchain) {
		// Origin from Haruno yo RFT27, principle of "Every action is a function, every object is in memory."; RFB19, RFV13 Rewrite£»
		parsed = false;
		chain = zalcof(NnodeChain);
		new (chain) NnodeChain(true);
		int state = 0;
		Tnode* crttn = 0, * tnext = 0;

		// Solve comment, Trim trailing or middle spaces;
		crttn = nchain.Root();
		while (crttn)
		{
			tnext = crttn->next;
			if (crttn->type == tok_comment ||
				crttn->type == tok_spaces &&
				crttn->row == crttn->left->row &&
				(!crttn->next || crttn->row == crttn->next->row))
				nchain.Remove(crttn);
			crttn = tnext;
		}

		// String cat (must on a line);
		crttn = nchain.Root();
		while (crttn) {
			tnext = crttn->next;
			while ((crttn->type == tok_string) && (crttn->next) && (crttn->next->type == tok_string))
			{
				tnext = crttn->next->next;
				srs(crttn->offs, StrHeapAppend((char*)crttn->offs, (char*)crttn->next->offs));
				nchain.Remove(crttn);
				
			}
			crttn = tnext;
		}

		// Discard any directive temporarily;
		crttn = nchain.Root();
		while (crttn) {
			tnext = crttn->next;
			if (crttn->type == tok_direct)
				nchain.Remove(crttn);
			crttn = tnext;
		}


		// ---- ---- ---- ---- LN ---> NS ---- ---- ---- ----
		// Restructure for nested

		nchain.~TnodeChain();
		memf(&nchain)
		parsed = true;
	}

	NestedParseUnit::~NestedParseUnit() {
		if (!this) return;
		if (chain) {
			chain->~NnodeChain();
			mfree(chain);
		}
	}
}


