﻿// ASCII C++-20 TAB4 CRLF
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

// Convert all operators into function calling form;

#include <new>
#include "../../../../../inc/cpp/unisym"
#include "../../../../../inc/cpp/string"
#include "../../../../../inc/cpp/dnode"
#include "../../../../../inc/cpp/nnode"

using namespace uni;

#define gettype(sym) (stepval(sym)->type)
// | whom must != 0 (0 means non-exist) for true, 
// v     so "0" is good to occupy.So for example, need not judge crt->left for `issuffix()`.
#define parawith(sym,whom) (maynotsym(gettype(whom)))// no &&!difline
// | whom may  != 0 (0 means non-exist) for true, so "0" is good to occupy
// v     so as above.
#define sepawith(sym,whom) (!whom || maysymbol(gettype(whom)))// no || difline

// size_t lleftt = crt->left->left ? crt->left->left->type : tok_any;
// size_t leftt = crt->left ? crt->left->type : tok_any;
// size_t rightt = crt->next->type;
// size_t rrightt = crt->next->next ? crt->next->next->type :tok_any;

inline static bool issuffix(uni::Nnode* crt) {
	return sepawith(crt, crt->next) && parawith(crt, crt->getLeft());// && sepawith(crt, crt->getLeft()->getLeft());
}
inline static bool isprefix(uni::Nnode* crt) {
	return sepawith(crt, crt->getLeft()) && parawith(crt, crt->next);// && sepawith(crt, crt->next->next);
}
inline static bool ismiddle(uni::Nnode* crt) {
	auto crtleft = crt->getLeft();
	bool a = parawith(crt, crtleft);
	// if (a) a = sepawith(crt, crtleft->getLeft());
	return a && parawith(crt, crt->next);// && sepawith(crt, crt->next->next);
}

typedef bool (*ParseOperatorFunction_t)(uni::Nnode*, uni::NnodeChain*, bool&);

const char* uni::StrIndexOperator(const char* str, uni::TokenOperator** operators, size_t count, bool left_to_right, stduint cond) {
	const char* idx;
	if (cond == 2) {
		for0(i, count) if (!StrCompare(str, (*operators)->idnop))
			return str;
		else ++ * operators;
	}
	else if (cond == 1) {
		for0(i, count) {
			if (idx = (left_to_right ? StrIndexString : StrIndexStringRight)(str, (*operators)->idnop))
				return idx;
			else ++ * operators;
		}
	}
	return nullptr;
}

//{TEMP} Suffix ANY/SYM/SPA/NL STR/NUM/IDEN/FUNC(2) SYM(suf ...) SYM/SPA/NL/NULL
//{TEMP} Prefix ANY/SYM/SPA/NL SYM(pre ...) STR/NUM/IDEN/FUNC(2) SYM/SPA/NL/NULL
//{TEMP} Middle ANY/SYM/SPA/NL STR/NUM/IDEN/FUNC(1) SYM(+-*/) STR/NUM/IDEN/FUNC(2) SYM/SPA/NL/NULL
static bool ParseOperatorGroup(uni::Nnode*& head, uni::NnodeChain* nc, uni::TokenOperatorGroup* tog, bool& exist_sym, bool LR_but_RL, stduint condi) {
	bool& op_suffix = LR_but_RL;
	uni::Nnode*& subfirst = head->pare ? head->pare->subf : nc->RootRef(),
		* crt = head, * tmp{ nullptr };
	const char* idx;
	uni::TokenOperator* tmpop = nullptr;
	//ploginfo("head %s", head->addr);
	if (subfirst) for (crt = (LR_but_RL ? subfirst : subfirst->Tail()); crt; crt = (LR_but_RL ? crt->next : crt->getLeft())) {
		if (crt->subf)
			ParseOperatorGroup(crt->subf, nc, tog, exist_sym, LR_but_RL, condi);
		if ((crt->type == tok_symbol) && (exist_sym = true)) {
			tmpop = tog->operators;
			uni::TokenOperator* tmpend = tmpop + tog->count;
			idx = StrIndexOperator(crt->addr, &tmpop, tog->count, LR_but_RL, condi);
			bool cont = true;
			if (tmpop) while (tmpop < tmpend && cont) {
				uni::Nnode* judge = 0;
				if (tmpop) {
					bool pass_divsym = false;
					if (true || tog->condition == 1 && tog->left_to_right == false || true) {
						char c = idx[StrLength(tmpop->idnop)];
						if (c && ascii_ispunct(c)) pass_divsym = true;
					}
					if (!pass_divsym) {
						nc->DivideSymbols(crt, StrLength(tmpop->idnop), idx - crt->addr);
						cont = false;
					}
					else tmpop++;
				}
				if (!idx) continue;
				if (condi == 2 && ismiddle(crt)) {
					auto newParent = nc->Append(StrHeap(stepval(tmpop)->ident), true, judge = crt->getLeft());
					*newParent->GetTnodeField() = *crt->GetTnodeField();
					AssignParallel(tmp, crt, nc->Adopt(newParent, crt->getLeft(), crt->next));
					if (tmpop->bindfn && nc->extn_field >= byteof(uni::mag_node_t)) {
						((uni::mag_node_t*)getExfield(*crt))->bind = tmpop->bindfn;
					}
					crt->GetTnodeField()->col = tmp->GetTnodeField()->col;
					nc->Remove(tmp);
					for (Nnode* subf = crt->subf; subf; subf = subf->next) {
						if (!ParseOperatorGroup(subf, nc, tog, exist_sym, LR_but_RL, condi)) return false;
					}
				}
				else if (condi == 1 && (op_suffix ? issuffix : isprefix)(crt)) { // Unary
					if (op_suffix) {
						nc->Exchange(crt, crt->getLeft());
						crt = crt->getLeft();
					}
					crt = nc->Adopt(crt, judge = crt->next)->ReheapString(stepval(tmpop)->ident);
					if (tmpop->bindfn && nc->extn_field >= byteof(uni::mag_node_t)) {
						((uni::mag_node_t*)getExfield(*crt))->bind = tmpop->bindfn;
					}
					for (Nnode* subf = crt->subf; subf; subf = subf->next) {
						if (!ParseOperatorGroup(subf, nc, tog, exist_sym, LR_but_RL, condi)) return false;
					}
				}
				if (head == judge)
					head = crt;
			}

		}
	}

	return true;
}

namespace uni {
	bool NestedParseUnit::ParseOperator(Nnode* head) {
		// rest tok_type{any, v:str, v:num, sym, v:iden, spa, func}
		if (!head || !TokenOperatorGroupChain) return true;
		bool exist_sym = true;
		bool state = true;
		Node* crt = TokenOperatorGroupChain->Root();
		TokenOperatorGroup* tog{ nullptr };
		if (crt) do if (tog = (TokenOperatorGroup*)crt->offs)
		{
			exist_sym = false;
			state = ParseOperatorGroup(head, this->chain, tog, exist_sym, tog->left_to_right, tog->condition);
		}
		while ((crt = crt->next) && (exist_sym && state));
		return state;
	}
}


