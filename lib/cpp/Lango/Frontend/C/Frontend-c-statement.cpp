// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend Statements
// Codifiers: @ArinaMgk
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

#include "../../../../../inc/cpp/Lango/Frontend/C/Frontend-c-internal.hpp"

namespace uni::lango {

namespace {

	bool IsSourceWordCharacter(char character) {
		return character == '_' ||
			(character >= '0' && character <= '9') ||
			(character >= 'A' && character <= 'Z') ||
			(character >= 'a' && character <= 'z');
	}

	void AppendSourceText(uni::String& text, const char* token) {
		if (!token || !*token) return;
		stduint length = text.getByteCount();
		if (length && IsSourceWordCharacter(text[length - 1]) &&
			IsSourceWordCharacter(token[0])) text << ' ';
		while (*token) text << *token++;
	}

	void AppendSourceNodes(uni::String& text, uni::Nnode* node) {
		for (; node; node = node->next) {
			if (node->addr) AppendSourceText(text, node->addr);
			if (node->subf) {
				text << '(';
				AppendSourceNodes(text, node->subf);
				text << ')';
			}
		}
	}

	void SaveStatementSource(FrontendCDebugInfo* debug_info, uni::Nnode* statement) {
		if (!debug_info || !statement || !statement->subf) return;
		uni::String text;
		AppendSourceNodes(text, statement->subf);
		text << ';';
		debug_info->Add(statement, text);
	}

	void SaveControlSource(FrontendCDebugInfo* debug_info, uni::Nnode* control) {
		if (!debug_info || !control) return;
		uni::String text;
		if (control->addr) AppendSourceText(text, control->addr);
		if (control->subf) {
			text << '(';
			if (control->addr && !StrCompare(control->addr, "for")) {
				uni::Nnode* clause = control->subf;
				for (stduint index = 0; index < 3; index++) {
					if (clause) {
						AppendSourceNodes(text, clause->subf);
						clause = clause->next;
					}
					if (index < 2) text << ';';
				}
			}
			else AppendSourceNodes(text, control->subf);
			text << ')';
		}
		debug_info->Add(control, text);
	}

	bool IsKeyword(const uni::Nnode* node, const char* keyword) {
		return node && node->addr && !StrCompare(node->addr, keyword);
	}

	bool PrepareDeclaration(uni::Nchain& syntax_tree, uni::Nnode* statement,
		const FrontendCDiagnostics& diagnostics) {
		uni::Nnode* type = statement ? statement->subf : nullptr;
		if (!type || type->type != tok_identy || !IsKeyword(type, "int")) return false;
		uni::Nnode* declarator = syntax_tree.Remove(type);
		if (!declarator) {
			diagnostics.Report(DiagnosticStage::Parser,
				"expected declarator after 'int'", statement);
			return false;
		}
		statement->type = ToToken(FrontendCToken::Declaration);
		return true;
	}

	void ClassifyControlNode(uni::Nnode* node, FrontendCDebugInfo* debug_info) {
		if (!node) return;
		if (node->type == tok_func && IsKeyword(node, "if")) {
			SaveControlSource(debug_info, node);
			node->type = ToToken(FrontendCToken::If);
		}
		else if (node->type == tok_func && IsKeyword(node, "while")) {
			SaveControlSource(debug_info, node);
			node->type = ToToken(FrontendCToken::While);
		}
		else if (node->type == tok_func && IsKeyword(node, "for")) {
			SaveControlSource(debug_info, node);
			node->type = ToToken(FrontendCToken::For);
		}
		else if (node->type == tok_identy && IsKeyword(node, "else")) {
			node->type = ToToken(FrontendCToken::Else);
		}
	}

	void AppendTrailingEmptyStatement(uni::Nchain& syntax_tree,
		uni::Nnode* statement) {
		if (!statement || !statement->subf) return;
		uni::Nnode* tail = statement->subf->Tail();
		if (tail->type != ToToken(FrontendCToken::If) &&
			tail->type != ToToken(FrontendCToken::Else) &&
			tail->type != ToToken(FrontendCToken::While) &&
			tail->type != ToToken(FrontendCToken::For)) return;
		uni::Nnode* empty = syntax_tree.Append(nullptr, false, tail);
		empty->type = ToToken(FrontendCToken::Empty);
		*empty->GetTnodeField() = *tail->GetTnodeField();
	}

	// Classify return statements before operator parsing. Removing the keyword
	// prevents unary and binary operators from treating it as an operand.
	bool PrepareBlock(uni::Nchain& syntax_tree, uni::Nnode* block,
		const FrontendCDiagnostics& diagnostics, FrontendCDebugInfo* debug_info) {
		if (!block || block->type != tok_block) {
			diagnostics.Report(DiagnosticStage::Parser,
				"expected function block", block);
			return false;
		}

		for (uni::Nnode* node = block->subf; node; node = node->next) {
			if (node->type == tok_block) {
				if (!PrepareBlock(syntax_tree, node, diagnostics, debug_info)) return false;
				continue;
			}
			if (node->type == tok_func &&
				(IsKeyword(node, "if") || IsKeyword(node, "while") ||
					IsKeyword(node, "for"))) {
				ClassifyControlNode(node, debug_info);
				continue;
			}
			if (node->type == tok_identy && IsKeyword(node, "else")) {
				ClassifyControlNode(node, debug_info);
				continue;
			}
			if (node->type != tok_statement) {
				diagnostics.Report(DiagnosticStage::Parser,
					"expected statement or block", node);
				return false;
			}

			SaveStatementSource(debug_info, node);
			if (!node->subf) {
				node->type = ToToken(FrontendCToken::Empty);
				continue;
			}
			if (node->subf->type == tok_identy && IsKeyword(node->subf, "int")) {
				if (!PrepareDeclaration(syntax_tree, node, diagnostics)) return false;
				continue;
			}
			for (uni::Nnode* child = node->subf; child; child = child->next)
				if (child->type == tok_block) {
					if (!PrepareBlock(syntax_tree, child, diagnostics, debug_info)) return false;
				}
				else {
					ClassifyControlNode(child, debug_info);
					if ((child->type == tok_identy || child->type == tok_func) &&
						IsKeyword(child, "return"))
						child->type = ToToken(FrontendCToken::Return);
				}
			uni::Nnode* first = node->subf;
			if (first && first->type == ToToken(FrontendCToken::Return) && first->subf) {
				node->type = ToToken(FrontendCToken::Return);
				// The statement owns the return keyword; keep return(...) as an
				// anonymous grouped expression for the common return lowering path.
				first->type = tok_func;
				srs(first->addr, nullptr);
			}
			else if (first && first->type == ToToken(FrontendCToken::Return)) {
				node->type = ToToken(FrontendCToken::Return);
				if (!syntax_tree.Remove(first)) {
					diagnostics.Report(DiagnosticStage::Parser,
						"return requires a value", node);
					return false;
				}
			}
			else AppendTrailingEmptyStatement(syntax_tree, node);
		}
		return true;
	}

	// Validate expression shape after operator parsing has folded each return
	// expression into one syntax-tree node.
	bool ValidateBlock(uni::Nnode* block, const FrontendCDiagnostics& diagnostics) {
		for (uni::Nnode* node = block ? block->subf : nullptr; node; node = node->next) {
			if (node->type == tok_block) {
				if (!ValidateBlock(node, diagnostics)) return false;
			}
			else if (node->type == ToToken(FrontendCToken::If) ||
				node->type == ToToken(FrontendCToken::Else) ||
				node->type == ToToken(FrontendCToken::While) ||
				node->type == ToToken(FrontendCToken::For) ||
				node->type == ToToken(FrontendCToken::Empty) ||
				node->type == ToToken(FrontendCToken::Declaration)) continue;
			else if (node->type == ToToken(FrontendCToken::Return) &&
				(!node->subf || node->subf->next)) {
				diagnostics.Report(DiagnosticStage::Parser,
					"invalid return expression", node);
				return false;
			}
			else if (node->type == tok_statement) {
				for (uni::Nnode* child = node->subf; child; child = child->next)
					if (child->type == tok_block &&
						!ValidateBlock(child, diagnostics)) return false;
			}
		}
		return true;
	}

}

void FrontendCDebugInfo::Add(const uni::Nnode* node, const uni::String& text) {
	FrontendCDebugSource source;
	source.node = node;
	source.text = text;
	sources.Append(source);
}

const uni::String* FrontendCDebugInfo::Find(const uni::Nnode* node) const {
	for (const FrontendCDebugSource& source : sources)
		if (source.node == node) return &source.text;
	return nullptr;
}

bool frontend_c_detail::Prepare(uni::Nchain& syntax_tree,
	const FrontendCDiagnostics& diagnostics, FrontendCDebugInfo* debug_info) {
	for (uni::Nnode* node = syntax_tree.Root(); node;) {
		if (node->type != tok_func || !node->addr) {
			diagnostics.Report(DiagnosticStage::Parser,
				"expected function definition", node);
			return false;
		}
		if (node->subf) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"function parameters are not supported yet", node);
			return false;
		}

		uni::Nnode* body = node->next;
		if (!body || body->type != tok_block) {
			diagnostics.Report(DiagnosticStage::Parser,
				"expected function body", body ? body : node);
			return false;
		}

		node->type = ToToken(FrontendCToken::Function);
		if (!PrepareBlock(syntax_tree, body, diagnostics, debug_info)) return false;
		node = body->next;
	}
	return true;
}

bool frontend_c_detail::Validate(uni::Nchain& syntax_tree,
	const FrontendCDiagnostics& diagnostics) {
	for (uni::Nnode* node = syntax_tree.Root(); node;) {
		uni::Nnode* body = node->next;
		if (node->type != ToToken(FrontendCToken::Function) ||
			!body || body->type != tok_block ||
			!ValidateBlock(body, diagnostics)) return false;
		node = body->next;
	}
	return true;
}

}


