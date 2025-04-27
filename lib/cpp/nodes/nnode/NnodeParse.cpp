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
#include "../../../../inc/cpp/string"
#include <new>

static bool NestedTokenParse(uni::NestedParseUnit* npu);

namespace uni {
	// Origin from Haruno yo RFT27, principle of "Every action is a function, every object is in memory."; RFB19, RFV13, 20240706 Rewrite
	NestedParseUnit::NestedParseUnit(const TnodeChain& tchain, NodeChain* TOGCChain, stduint extn_fielen) : TokenOperatorGroupChain(TOGCChain) {
		chain = zalcof(NnodeChain);
		new (chain) NnodeChain(true);
		chain->extn_field = maxof(byteof(TnodeField), extn_fielen);

		// Structure for nested ---- Line ---> Nest ----
		if (auto crttn = (Tnode*)tchain.Root()) do chain->Append(crttn); while (crttn = crttn->next);
	}

	// classic and outdated
	bool NestedParseUnit::NnodeParse(Nnode* tnod, NnodeChain* chain, bool merge_parensd) {
		if (!tnod) return true;

		bool state = true;
		Nnode* crt = tnod;
		bool exist_sym{ false };
		int cases;
		char c;
		// Below: for ( )
		stduint crtnest = 0;
		char crtnest_type = 0;// 0 or ( or [
		char crtnest_endp = 0;//(helpful)
		Nnode* last_parens = 0;

		// Process what are not prefix, suffix or midfix, whose priority is the highest
		// - paren     ()
		// - subscript []
		while (crt) {
			if (crt->type == tok_symbol) for0(i, StrLength(crt->addr)) {
				c = crt->addr[i];
				if (!crtnest_type && (c == '(' || c == '[') || (c == crtnest_type))
				{
					crtnest++;
					if (crtnest == 1) {
						last_parens = crt;
						crtnest_type = c;
						crtnest_endp = (c == '(') ? ')' : ']';
						//
						cases = chain->DivideSymbols(crt, 1, i);
						if (crtnest == 1 && (cases == NNODE_DIVSYM_TAIL || cases == NNODE_DIVSYM_MIDD))
						{
							exist_sym = true;
							if (tnod->getLeft()) tnod = tnod->getLeft();
						}
						break;
					}
				}
				else if (c == crtnest_endp)
				{
					crtnest--;
					if (crtnest == 0) {
						crtnest_type = crtnest_endp = 0;
						cases = chain->DivideSymbols(crt, 1, i);
						if (cases == NNODE_DIVSYM_HEAD || cases == NNODE_DIVSYM_MIDD) exist_sym = true;
						Nnode* fn = last_parens->getLeft();
						bool anonymity = !(last_parens->getLeft() && last_parens->getLeft()->type == tok_identy);
						//
						if (c == ')') {
							// a(b), +(b)
							if (anonymity) fn = chain->Append(nullptr, true, last_parens);
							NnodeBlock(fn, last_parens->next, crt->getLeft())->type = tok_func;// chain->Adopt(fn, last_parens->next, crt->getLeft());
							chain->Remove(last_parens); if (last_parens == tnod) tnod = fn;
							chain->Remove(crt);
						}
						else {// ]
							// [], +[a] => fn(NULL,a), a [ b ] => fn( a b )
							// e,g, (-a[1][2])[3], [a]([b][v]), ([(a)])
							bool valued = (last_parens->getLeft() && last_parens->getLeft()->type == tok_func);
							if (anonymity && !valued) (fn = chain->Append(NULL, true, last_parens))->type = tok_string;
							chain->Exchange(fn, last_parens); xchgptr(fn, last_parens);// xchg a and [, now fn is a
							NnodeBlock(last_parens, last_parens->next, crt->getLeft())->type = tok_func;
							srs(last_parens->addr, StrHeap("operator[]"));
							chain->Remove(crt);
							fn = last_parens;
						}
						crt = fn;
						if (!NnodeParse(fn->subf, chain))
						{
							state = false;
							goto endo;
						}
						if (c == ')' && merge_parensd && fn->type == tok_func && !fn->addr) {
							if (fn->subf && fn->subf->next == 0) {
								// assert fn->subf->getLeft() == 0
								//{TODO} fn->subf->pare = fn->pare;
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
					else if (!last_parens) { // crtnest > 0
						//fprintf(stderr, "Unmatched parenthesis at line %" PRIuPTR ".", crt->row);
						state = false;
						goto endo;
					}
				}
			}
			if (!crt) return true;
			crt = crt->next;
		}
		if (crtnest) {
			state = false;
			goto endo;
		}
		state = ParseOperator(tnod, chain);
	endo:
		if (!state) {
			chain->~Nchain();
		}
		return state;
	}

	NestedParseUnit::~NestedParseUnit() {
		if (!this) return;
		// if (TokenOperatorGroupChain) {
		// 	TokenOperatorGroupChain->~NodeChain();
		// 	mfree(TokenOperatorGroupChain);
		// }
		if (chain) {
			chain->~NnodeChain();
			mfree(chain);
		}
	}

}
