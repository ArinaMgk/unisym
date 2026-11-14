// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Compound Literal Syntax
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
	bool IsSubscript(const uni::Nnode* node) {
		return node && node->type == tok_func && node->addr &&
			!StrCompare(node->addr, "operator[]");
	}

	bool LooksLikeTypeNameStart(const uni::Nnode* node) {
		if (!node) return false;
		if (node->type == tok_identy && node->addr) return true;
		if (node->type != tok_func || !node->subf) return false;
		if (!node->addr || !StrCompare(node->addr, "operator[]"))
			return LooksLikeTypeNameStart(node->subf);
		return false;
	}

	uni::Nnode* WrapCompoundLiteralPair(uni::Nchain& syntax_tree,
		uni::Nnode*& head, uni::Nnode* type_name, uni::Nnode* initializer,
		bool contiguous) {
		uni::Nnode* compound = syntax_tree.Append(nullptr, true, type_name);
		*compound->GetTnodeField() = *type_name->GetTnodeField();
		if (head == type_name) head = compound;
		if (contiguous) syntax_tree.Adopt(compound, type_name, initializer);
		else {
			syntax_tree.Adopt(compound, type_name, type_name);
			type_name->next = initializer;
			initializer->left = type_name;
			initializer->next = nullptr;
		}
		compound->type = ToToken(FrontendCToken::CompoundLiteral);
		return compound;
	}

	bool PreserveCompoundLiteralList(uni::Nchain& syntax_tree, uni::Nnode*& head,
		const uni::Nnode* end = nullptr) {
		for (uni::Nnode* node = head; node && node != end;) {
			if (node->subf && !PreserveCompoundLiteralList(
				syntax_tree, node->subf)) return false;
			uni::Nnode* initializer = node->next;
			if (node->type == tok_func && !node->addr && node->subf &&
				LooksLikeTypeNameStart(node->subf) &&
				IsSubscript(initializer) && initializer->subf &&
				initializer->subf->type == tok_block &&
				initializer->subf->next) {
				uni::Nnode* subscript = initializer;
				initializer = subscript->subf;
				uni::Nnode* index = initializer->next;
				if (initializer->subf && !PreserveCompoundLiteralList(
					syntax_tree, initializer->subf)) return false;
				subscript->subf = index;
				index->left = subscript;
				initializer->next = nullptr;
				uni::Nnode* compound = WrapCompoundLiteralPair(
					syntax_tree, head, node, initializer, false);
				syntax_tree.Exchange(compound, subscript);
				index = compound->subf;
				syntax_tree.Adopt(compound, subscript, subscript);
				subscript->next = index;
				index->left = subscript;
				node = compound->next;
				continue;
			}
			if (node->type != tok_func || node->addr || !node->subf ||
				!LooksLikeTypeNameStart(node->subf) ||
				!initializer || initializer->type != tok_block) {
				node = node->next;
				continue;
			}
			if (initializer->subf && !PreserveCompoundLiteralList(
				syntax_tree, initializer->subf)) return false;
			uni::Nnode* compound = WrapCompoundLiteralPair(
				syntax_tree, head, node, initializer, true);
			node = compound->next;
		}
		return true;
	}

	bool IsAssignmentStart(const uni::Nnode* node) {
		return node && node->type == tok_symbol && node->addr &&
			node->addr[0] == '=' && node->addr[1] != '=';
	}

	bool ContainsSemicolon(const uni::Nnode* node) {
		return node && node->type == tok_symbol && node->addr &&
			StrIndexChar(node->addr, ';');
	}

	bool IsIncompleteArrayType(const FrontendCType& type) {
		if (!type.IsArray() || type.ArrayDimension(0)) return false;
		for (stduint index = 1; index < type.ArrayDimensionCount(); index++)
			if (!type.ArrayDimension(index)) return false;
		return true;
	}

	bool PrepareCompoundLiteralList(uni::Nchain& syntax_tree, uni::Nnode* node,
		const FrontendCDiagnostics& diagnostics,
		const FrontendCTargetInfo& target, FrontendCTypeInfo& type_info) {
		for (; node; node = node->next) {
			if (node->type == ToToken(FrontendCToken::CompoundLiteral)) {
				uni::Nnode* type_name = node->subf;
				uni::Nnode* initializer = type_name ? type_name->next : nullptr;
				FrontendCType type;
				if (!type_name || type_name->type != tok_func || type_name->addr ||
					!type_name->subf || !initializer || initializer->next ||
					initializer->type != tok_block || !ParseTypeName(
						type_name->subf, type, target, type_info)) {
					diagnostics.Report(DiagnosticStage::Parser,
						"compound literal requires a valid type name", node);
					return false;
				}
				if (type.IsVoid() || type.IsFunction()) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"compound literal requires an object type", node);
					return false;
				}
				if (type.HasIncompleteArray() && !IsIncompleteArrayType(type)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"compound literal has an invalid incomplete array type", node);
					return false;
				}
				if (!type.HasIncompleteArray() && !type_info.SizeBits(type, target)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"compound literal requires a complete object type", node);
					return false;
				}
				if (!PrepareInitializer(syntax_tree, initializer, diagnostics) ||
					!PrepareCompoundLiteralList(syntax_tree, initializer->subf,
						diagnostics, target, type_info)) return false;
				syntax_tree.Remove(type_name);
				type_info.Add(node, type);
				continue;
			}
			if (node->subf && !PrepareCompoundLiteralList(syntax_tree, node->subf,
				diagnostics, target, type_info)) return false;
		}
		return true;
	}
}

bool PreserveCompoundLiteralSyntax(uni::Nchain& syntax_tree) {
	for (uni::Nnode* node = syntax_tree.Root(); node;) {
		if (IsAssignmentStart(node)) {
			// Only scan one file-scope initializer range. Scanning the complete
			// translation unit would confuse a function return declarator followed
			// by its body with a compound literal.
			uni::Nnode* end = node->next;
			while (end && !ContainsSemicolon(end)) end = end->next;
			if (node->next != end && !PreserveCompoundLiteralList(
				syntax_tree, node->next, end)) return false;
			node = end ? end->next : nullptr;
			continue;
		}
		if (node->type == tok_block && node->subf &&
			!PreserveCompoundLiteralList(syntax_tree, node->subf)) return false;
		node = node->next;
	}
	return true;
}

bool PrepareCompoundLiterals(uni::Nchain& syntax_tree,
	const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
	FrontendCTypeInfo& type_info) {
	return PrepareCompoundLiteralList(syntax_tree, syntax_tree.Root(),
		diagnostics, target, type_info);
}

}
