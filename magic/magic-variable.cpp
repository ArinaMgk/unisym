// ASCII C++ TAB4 LF
// Docutitle: Magice Operator
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

#include "./magice.hpp"


#include "../inc/cpp/parse.hpp"

using namespace uni;

//

struct VariableElement {
	rostr iden;
	struct {
		stduint size;// default 8
	};
	stduint local_stack_offset;
};

VariableElement* LocVari(NodeChain& variables, rostr iden) {
	for (auto var = variables.Root(); var; var = var->next) {
		Letvar(element, VariableElement*, var->offs);
		if (!StrCompare(iden, element->iden)) return element;
	}
	return nullptr;
}

void AddVari(NodeChain& variables, rostr iden) {
	auto element = LocVari(variables, iden);
	if (element) {
		if (StrCompare(iden, element->iden)) srs(element->iden, StrHeap(iden));
	}
	else {
		VariableElement* new_elm = zalcof(VariableElement);
		new_elm->iden = StrHeap(iden);
		new_elm->local_stack_offset = 8 * variables.Count();
		variables.Append(new_elm);
		//ploginfo("appended %s", new_elm->iden);
	}
}

void DelVari(pureptr_t p) {
	Letvar(element, VariableElement*, ((Node*)p)->offs);
	mfree(element->iden);
	mfree(element);
}

static void ListVariSub(NodeChain& variables, Nnode* beg) {
	for (Nnode* crt = beg; crt; crt = crt->next) {
		if (crt->type == tok_identy && crt->addr) {
			AddVari(variables, crt->addr);
		}
		if (crt->subf) ListVariSub(variables, crt->subf);
	}
}


Variable::Variable() : variables(DelVari) {
	
}

// public:

void Variable::List(uni::Nchain* nchain) {
	ListVariSub(variables, nchain->Root());
}

stdsint Variable::GetOffset(rostr iden) {
	VariableElement* elm = LocVari(variables, iden);
	return elm ? (stdsint)elm->local_stack_offset : -1LL;
}
