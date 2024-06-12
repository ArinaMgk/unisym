// UTF-8 C++-20 TAB4 CRLF
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

#include "../../../../inc/cpp/unisym"
#include "../../../../inc/cpp/nnode"
#include <new>

static bool NestedTokenParse(uni::NestedParseUnit* npu);

namespace uni {
	NestedParseUnit::NestedParseUnit(TnodeChain& tchain, NodeChain* TOGCChain) : TokenOperatorGroupChain(TOGCChain) {
		linemn_no = column_no = 0;
		msg_fail = 0;
		TokenOperatorGroupChain->func_free = NodeFreeSimple;
		// Origin from Haruno yo RFT27, principle of "Every action is a function, every object is in memory."; RFB19, RFV13 Rewrite
		parsed = false;
		chain = zalcof(NnodeChain);
		new (chain) NnodeChain(true);
		int state = 0;
		Tnode* crttn = 0;
		union { Tnode* tnext{nullptr}; Nnode* nnext; };

		// Solve comment, Trim trailing or middle spaces;
		if (crttn = tchain.Root()) while (crttn = 
			(crttn->type == tok_comment ||
			crttn->type == tok_spaces &&
			crttn->row == crttn->left->row &&
			(!crttn->next || crttn->row == crttn->next->row)) ?
			tchain.Remove(crttn) : crttn->next);

		// String cat (must on a line);
		if (crttn = tchain.Root()) do {
			while ((crttn->type == tok_string) && (crttn->next) && (crttn->next->type == tok_string))
			{
				srs(crttn->offs, StrHeapAppend((char*)crttn->offs, (char*)crttn->next->offs));
				tchain.Remove(crttn->next);
			}
		} while (crttn = crttn->next);

		// Discard any directive temporarily;
		crttn = tchain.Root();
		while (crttn)
			crttn = (crttn->type == tok_direct) ? tchain.Remove(crttn) : crttn->next;

		// ---- ---- ---- ---- Line ---> Nest ---- ---- ---- ----
		tchain.Remove(tchain.Root());

		// Restructure for nested
		tchain.Onfree(0, false);
		crttn = tchain.Root();
		while (crttn)
		{
			chain->Append(crttn);
			crttn = tchain.Remove(crttn);
		}

		tchain.~TnodeChain();
		memf(&tchain)
		parsed = true;
	}


	bool NestedParseUnit::NnodeParse(Nnode* tnod, NnodeChain* chain, bool merge_parensd) {
		if (!tnod) return true;

		bool state = true;
		Nnode* crt = tnod;
		bool exist_sym{ false };
		int cases;
		char c;
		// Below: for ( )
		stduint crtnest = 0;
		Nnode* last_parens = 0;

		// E.g. 0+func(0w0)
		//      0+[ ... ], [...]={func, {0}{w}{0}}
		while (crt) {
			if (crt->type == tok_symbol) for0(i, StrLength(crt->addr)) {
				c = crt->addr[i];
				if (c == '(') {
					crtnest++;
					if (crtnest == 1) {
						last_parens = crt;
						cases = chain->DivideSymbols(crt, 1, i);
						if (crtnest == 1 && (cases == NNODE_DIVSYM_TAIL || cases == NNODE_DIVSYM_MIDD)) exist_sym = true;
						break;
					}
				}
				else if (c == ')')
				{
					// do not care only one item in the block
					if (!crtnest-- && !last_parens) {
						//fprintf(stderr, "Unmatched parenthesis at line %" PRIuPTR ".", crt->row);
						return false;
					}
					if (crtnest == 0)
					{
						cases = chain->DivideSymbols(crt, 1, i);
						if (cases == NNODE_DIVSYM_HEAD || cases == NNODE_DIVSYM_MIDD) exist_sym = true;
						Nnode* fn = last_parens->left;// assume not anonymity
						if (!(last_parens->left && last_parens->left->type == tok_identy && last_parens->left->row == last_parens->row))// anonymity
							fn = chain->Insert(last_parens, true);
						chain->Adopt(fn, last_parens->next, crt->left);
						chain->Remove(last_parens); if (last_parens == tnod) tnod = fn;
						chain->Remove(crt);
						crt = fn;
						if (!NnodeParse(fn->subf, chain))
							return false;
						if (merge_parensd && fn->type == tok_func && !fn->addr) {
							if (fn->subf && fn->subf->next == 0) {
								// assert fn->subf->left == 0
								fn->subf->pare = fn->pare;
								fn->subf->left = fn;
								if (fn->subf->next = fn->next)
									fn->next->left = fn->subf;
								fn->next = fn->subf;
								fn->subf = 0;
							}
							crt = chain->Remove(fn);
							if (fn == tnod) tnod = crt;
						}	
						/// exist_sym = 0; ま
						break;
					}
				}
			}
			if (!crt) return true;
			crt = crt->next;
			if (crt && (crt->row != crt->left->row)) last_parens = 0;
		}
		//{TODO} if (crtnest) erro("Match error");
		return ParseOperator(tnod, chain);
	}

	NestedParseUnit::~NestedParseUnit() {
		if (!this) return;
		if (TokenOperatorGroupChain) {
			TokenOperatorGroupChain->~NodeChain();
			mfree(TokenOperatorGroupChain);
		}
		if (chain) {
			chain->~NnodeChain();
			mfree(chain);
		}
	}

}
