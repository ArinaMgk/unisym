// ASCII C++ TAB4 LF
// Docutitle: Magice Statement (div. by ';')
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

namespace {

	// Classify return statements before operator parsing. Removing the keyword
	// prevents unary and binary operators from treating it as an operand.
	bool PrepareBlock(uni::Nchain& syntax_tree, uni::Nnode* block) {
		if (!block || block->type != tok_block) {
			plogerro("C frontend: expected function block");
			return false;
		}

		for (uni::Nnode* node = block->subf; node; node = node->next) {
			if (node->type == tok_block) {
				if (!PrepareBlock(syntax_tree, node)) return false;
				continue;
			}
			if (node->type != tok_statement) {
				plogerro("C frontend: expected statement or block");
				return false;
			}

			uni::Nnode* first = node->subf;
			if (first && first->type == tok_identy && first->addr &&
				!StrCompare(first->addr, "return")) {
				node->type = magict_sta_return;
				if (!syntax_tree.Remove(first)) {
					plogerro("C frontend: return requires a value");
					return false;
				}
			}
		}
		return true;
	}

	// Validate expression shape after operator parsing has folded each return
	// expression into one syntax-tree node.
	bool ValidateBlock(uni::Nnode* block) {
		for (uni::Nnode* node = block ? block->subf : nullptr; node; node = node->next) {
			if (node->type == tok_block) {
				if (!ValidateBlock(node)) return false;
			}
			else if (node->type == magict_sta_return &&
				(!node->subf || node->subf->next)) {
				plogerro("C frontend: invalid return expression");
				return false;
			}
		}
		return true;
	}

}

bool FrontendCPrepare(uni::Nchain& syntax_tree) {
	for (uni::Nnode* node = syntax_tree.Root(); node;) {
		if (node->type != tok_func || !node->addr) {
			plogerro("C frontend: expected function definition");
			return false;
		}
		if (node->subf) {
			plogerro("C frontend: function parameters are not supported yet");
			return false;
		}

		uni::Nnode* body = node->next;
		if (!body || body->type != tok_block) {
			plogerro("C frontend: expected function body after %s", node->addr);
			return false;
		}

		node->type = magict_function;
		if (!PrepareBlock(syntax_tree, body)) return false;
		node = body->next;
	}
	return true;
}

bool FrontendCValidate(uni::Nchain& syntax_tree) {
	for (uni::Nnode* node = syntax_tree.Root(); node;) {
		uni::Nnode* body = node->next;
		if (node->type != magict_function || !body || body->type != tok_block ||
			!ValidateBlock(body)) return false;
		node = body->next;
	}
	return true;
}


