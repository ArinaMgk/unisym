// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Initializer Syntax
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2026 ArinaMgk

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

#include "../../../../../inc/cpp/Lango/Frontend/C/Frontend-c-internal.hpp"

namespace uni::lango::frontend_c_detail {

namespace {
	bool IsSymbol(const uni::Nnode* node, const char* symbol) {
		return node && node->type == tok_symbol && node->addr &&
			!StrCompare(node->addr, symbol);
	}

	bool IsIndexDesignator(const uni::Nnode* node) {
		return node && node->type == tok_func && node->addr &&
			!StrCompare(node->addr, "operator[]");
	}

	bool IsMemberDesignator(const uni::Nnode* node) {
		if (!IsSymbol(node, ".")) return false;
		const uni::Nnode* member = node->next;
		if (member && member->type == tok_identy && member->addr) return true;
		return IsIndexDesignator(member) && member->subf &&
			member->subf->type == tok_identy && member->subf->addr &&
			member->subf->next;
	}

	bool IsDesignator(const uni::Nnode* node) {
		return IsMemberDesignator(node) || IsIndexDesignator(node);
	}

	void NormalizeCommaSymbols(uni::Nchain& syntax_tree, uni::Nnode* head) {
		bool changed;
		do {
			changed = false;
			for (uni::Nnode* node = head; node; node = node->next) {
				if (node->type != tok_symbol || !node->addr) continue;
				const char* comma = StrIndexChar(node->addr, ',');
				if (!comma || StrLength(node->addr) == 1) continue;
				syntax_tree.DivideSymbols(node, 1, comma - node->addr);
				changed = true;
				break;
			}
		} while (changed);
	}

	bool PrepareInitializerNode(uni::Nchain& syntax_tree, uni::Nnode* node,
		const FrontendCDiagnostics& diagnostics);

	bool PrepareInitializerBlock(uni::Nchain& syntax_tree, uni::Nnode* block,
		const FrontendCDiagnostics& diagnostics) {
		NormalizeCommaSymbols(syntax_tree, block->subf);
		uni::Nnode* node = block->subf;
		while (node) {
			if (IsSymbol(node, ",")) {
				node = node->next;
				continue;
			}
			if (!IsDesignator(node)) {
				uni::Nnode* value_last = node;
				while (value_last->next && !IsSymbol(value_last->next, ","))
					value_last = value_last->next;
				uni::Nnode* next = value_last->next;
				for (uni::Nnode* value = node; value; value = value->next) {
					if (!PrepareInitializerNode(syntax_tree, value, diagnostics))
						return false;
					if (value == value_last) break;
				}
				node = next;
				continue;
			}

			uni::Nnode* first = node;
			uni::Nnode* cursor = node;
			while (IsDesignator(cursor)) {
				if (IsMemberDesignator(cursor)) {
					uni::Nnode* member = cursor->next;
					uni::Nnode* member_index = nullptr;
					if (IsIndexDesignator(member) && member->subf &&
						member->subf->type == tok_identy && member->subf->addr &&
						member->subf->next) {
						member_index = member;
						member = member->subf;
					}
					if (!member || member->type != tok_identy || !member->addr) {
						diagnostics.Report(DiagnosticStage::Parser,
							"member designator requires an identifier", cursor);
						return false;
					}
					cursor->type = ToToken(FrontendCToken::MemberDesignator);
					srs(cursor->addr, StrHeap(member->addr));
					if (member_index)
						member_index->subf = syntax_tree.Remove(member);
					else syntax_tree.Remove(member);
					cursor = cursor->next;
					continue;
				}
				if (!cursor->subf) {
					diagnostics.Report(DiagnosticStage::Parser,
						"array designator requires an index", cursor);
					return false;
				}
				if (cursor->subf->next && !cursor->subf->addr &&
					!cursor->subf->subf)
					cursor->subf = syntax_tree.Remove(cursor->subf);
				cursor->type = ToToken(FrontendCToken::IndexDesignator);
				srs(cursor->addr, nullptr);
				cursor = cursor->next;
			}

			if (!IsSymbol(cursor, "=")) {
				diagnostics.Report(DiagnosticStage::Parser,
					"initializer designation requires '='", cursor ? cursor : first);
				return false;
			}
			uni::Nnode* equal = cursor;
			uni::Nnode* value = equal->next;
			if (!value || IsSymbol(value, ",")) {
				diagnostics.Report(DiagnosticStage::Parser,
					"initializer designation requires a value", equal);
				return false;
			}
			uni::Nnode* value_last = value;
			while (value_last->next && !IsSymbol(value_last->next, ","))
				value_last = value_last->next;
			for (uni::Nnode* child = value; child; child = child->next) {
				if (!PrepareInitializerNode(syntax_tree, child, diagnostics)) return false;
				if (child == value_last) break;
			}

			uni::Nnode* value_wrapper = syntax_tree.Append(nullptr, true, value);
			value_wrapper->type = ToToken(FrontendCToken::InitializerValue);
			*value_wrapper->GetTnodeField() = *value->GetTnodeField();
			syntax_tree.Adopt(value_wrapper, value, value_last, false);
			syntax_tree.Remove(equal);

			uni::Nnode* designated = syntax_tree.Append(nullptr, true, first);
			designated->type = ToToken(FrontendCToken::DesignatedInitializer);
			*designated->GetTnodeField() = *first->GetTnodeField();
			if (block->subf == first) block->subf = designated;
			syntax_tree.Adopt(designated, first, value_wrapper, false);
			node = designated->next;
		}
		return true;
	}

	bool PrepareInitializerNode(uni::Nchain& syntax_tree, uni::Nnode* node,
		const FrontendCDiagnostics& diagnostics) {
		if (!node) return true;
		if (node->type == tok_block)
			return PrepareInitializerBlock(syntax_tree, node, diagnostics);
		for (uni::Nnode* child = node->subf; child; child = child->next)
			if (!PrepareInitializerNode(syntax_tree, child, diagnostics)) return false;
		return true;
	}
}

bool PrepareInitializer(uni::Nchain& syntax_tree, uni::Nnode* initializer,
	const FrontendCDiagnostics& diagnostics) {
	return PrepareInitializerNode(syntax_tree, initializer, diagnostics);
}

}
