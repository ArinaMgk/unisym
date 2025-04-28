// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Data) Token Node Parse Instructions and Directives
// Codifiers: @ArinaMgk 
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

#include "../../../inc/cpp/parse.hpp"

using namespace uni;

static bool is_declaration_assignment_tail(Nnode* left) {
	if (left->type == tok_identy) return true;// C++ {}-assignment except "void (*f)(...) { NULL };"
	if (left->type == tok_symbol &&
		(StrCharLast(left->addr) == '=' || StrCharLast(left->addr) == ']'))
	{
		return true;
	}
	return false;
}

static void ParseStatements_CPL(Nchain& nchain, Nnode* beg_nod) {
	// Process ;
	int cases;
	Nnode* left0_nod = beg_nod;
	for (Nnode* crt = beg_nod; crt; crt = crt->next) {
		if (crt->type == tok_symbol && crt->addr) for0(i, StrLength(crt->addr)) {
			char ch = crt->addr[i];
			if (ch != ';') continue;
			cases = nchain.DivideSymbols(crt, 1, i);
			Nnode* fn = nchain.Append(nullptr, true, left0_nod);
			NnodeBlock(fn, left0_nod, crt->getLeft())->type = tok_statement;// chain->Adopt
			fn->GetTnodeField()->col = left0_nod->GetTnodeField()->col;
			fn->GetTnodeField()->row = left0_nod->GetTnodeField()->row;
			nchain.Remove(crt);
			crt = fn;
			left0_nod = crt->next;
			break;
		}
		// consider declaration assignment
		else if (crt->type == tok_block && !crt->getParent() && !is_declaration_assignment_tail(crt->left)) {
			//{} check: FUN+BLK without pare (only think FileScope can exist Functions)
			// 
			//if (!is_declaration_assignment_tail(crt->left)) {
			//	Nnode* fn = nchain.Append(nullptr, true, left0_nod);
			//	NnodeBlock(fn, left0_nod, crt->getLeft())->type = tok_statement;// chain->Adopt
			//	fn->GetTnodeField()->col = left0_nod->GetTnodeField()->col;
			//	fn->GetTnodeField()->row = left0_nod->GetTnodeField()->row;
			//	left0_nod = crt->next;
			//}
			left0_nod = crt->next;
		}
	}
	// [may Just a Value]
	//if (left0_nod && left0_nod != beg_nod && left0_nod->next) {
	//	Nnode* fn = nchain.Append(nullptr, true, left0_nod);
	//	NnodeBlock(fn, left0_nod, left0_nod->Tail())->type = tok_statement;// chain->Adopt
	//	fn->GetTnodeField()->col = left0_nod->GetTnodeField()->col;
	//	fn->GetTnodeField()->row = left0_nod->GetTnodeField()->row;
	//}
}

static void ParseBlocks_CPL(Nchain& nchain, Nnode* beg_nod) {
	// Process { and }
	stduint crt_nest = 0;
	Nnode* left0_nod = nullptr;
	int cases;
	for (Nnode* crt = beg_nod; crt; crt = crt->next) {
		if (crt->type == tok_symbol && crt->addr) for0(i, StrLength(crt->addr)) {
			char ch = crt->addr[i];
			if (ch == '{' && !crt_nest++) {
				left0_nod = crt;
				cases = nchain.DivideSymbols(crt, 1, i);
				break;
			}
			else if (ch == '}' && !--crt_nest) {
				cases = nchain.DivideSymbols(crt, 1, i);
				Nnode* fn = nchain.Append(nullptr, true, left0_nod);
				NnodeBlock(fn, left0_nod->next, crt->getLeft())->type = tok_block;// chain->Adopt
				fn->GetTnodeField()->col = left0_nod->GetTnodeField()->col;
				fn->GetTnodeField()->row = left0_nod->GetTnodeField()->row;
				nchain.Remove(left0_nod);
				if (left0_nod == crt) crt = fn;
				nchain.Remove(crt);
				crt = fn;
				if (fn->subf) ParseBlocks_CPL(nchain, fn->subf);
				break;
			}
		}
	}
}

static void ParseStatements_Sub(Nchain& nchain, Nnode* beg_nod) {
	Nnode* crt = beg_nod;
	while (crt) {
		if (crt->subf) ParseStatements_Sub(nchain, crt->subf);
		crt = crt->next;
	}
	ParseStatements_CPL(nchain, beg_nod);
}

void uni::NestedParseUnit::ParseBlockStatements_CPL(Nnode* beg_nod) {
	Nchain& nchain = *self.GetNetwork();
	ParseBlocks_CPL(nchain, beg_nod);
	//{} - ( ) Parens and [ ] Brackets
	ParseStatements_Sub(nchain, beg_nod);
}

