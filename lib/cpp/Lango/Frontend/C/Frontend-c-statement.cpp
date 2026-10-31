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
			if (node->type == tok_string) {
				text << '"';
				if (node->addr) {
					const char* source = node->addr;
					while (*source) text << *source++;
				}
				text << '"';
			}
			else if (node->addr) AppendSourceText(text, node->addr);
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

	bool IsTypeSpecifier(const uni::Nnode* node) {
		if (!node || node->type != tok_identy || !node->addr) return false;
		return IsKeyword(node, "const") ||
			IsKeyword(node, "signed") || IsKeyword(node, "unsigned") ||
			IsKeyword(node, "char") || IsKeyword(node, "short") ||
			IsKeyword(node, "int") || IsKeyword(node, "long") ||
			IsKeyword(node, "void") || IsKeyword(node, "struct");
	}

	// Consume one C99 scalar type-specifier sequence before its declarator.
	bool ParseTypeSpecifiers(uni::Nchain& syntax_tree, uni::Nnode*& node,
		FrontendCType& type, const FrontendCDiagnostics& diagnostics,
		const FrontendCTargetInfo& target, FrontendCTypeInfo& type_info) {
		unsigned signed_count = 0, unsigned_count = 0, char_count = 0;
		unsigned short_count = 0, int_count = 0, long_count = 0, void_count = 0;
		unsigned const_count = 0;
		uni::Nnode* first = node;
		uni::Nnode* after_type = node;
		while (IsKeyword(after_type, "const")) {
			const_count++;
			after_type = after_type->next;
		}
		if (IsKeyword(after_type, "struct")) {
			uni::Nnode* tag = after_type->next;
			if (const_count > 1 || !tag || tag->type != tok_identy || !tag->addr) {
				diagnostics.Report(DiagnosticStage::Parser,
					"expected struct tag", after_type);
				return false;
			}
			stduint record_id = type_info.FindRecord(tag->addr);
			if (!record_id) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"use of undefined struct type", tag);
				return false;
			}
			after_type = tag->next;
			while (node != after_type) node = syntax_tree.Remove(node);
			type = FrontendCType();
			type.record_id = record_id;
			type.base_qualifiers.is_const = const_count != 0;
			return true;
		}
		after_type = first;
		const_count = 0;
		while (IsTypeSpecifier(after_type) && !IsKeyword(after_type, "struct")) {
			if (IsKeyword(after_type, "const")) const_count++;
			else if (IsKeyword(after_type, "signed")) signed_count++;
			else if (IsKeyword(after_type, "unsigned")) unsigned_count++;
			else if (IsKeyword(after_type, "char")) char_count++;
			else if (IsKeyword(after_type, "short")) short_count++;
			else if (IsKeyword(after_type, "int")) int_count++;
			else if (IsKeyword(after_type, "long")) long_count++;
			else if (IsKeyword(after_type, "void")) void_count++;
			after_type = after_type->next;
		}
		if (after_type == first) return false;
		if (const_count > 1 || signed_count > 1 || unsigned_count > 1 || void_count > 1 ||
			!(signed_count || unsigned_count || char_count || short_count ||
				int_count || long_count || void_count) ||
			(signed_count && unsigned_count) || char_count > 1 ||
			short_count > 1 || int_count > 1 || long_count > 2 ||
			(char_count && (short_count || int_count || long_count)) ||
			(short_count && long_count) ||
			(void_count && (signed_count || unsigned_count || char_count ||
				short_count || int_count || long_count))) {
			diagnostics.Report(DiagnosticStage::Parser,
				"invalid integer type specifier sequence", first);
			return false;
		}
		while (node != after_type) node = syntax_tree.Remove(node);
		type = FrontendCType();
		type.is_void = void_count != 0;
		type.base_qualifiers.is_const = const_count != 0;
		type.is_signed = unsigned_count ? false :
			(char_count && !signed_count ? target.char_is_signed : true);
		if (char_count) type.integer_rank = FrontendCIntegerRank::Char;
		else if (short_count) type.integer_rank = FrontendCIntegerRank::Short;
		else if (long_count == 1) type.integer_rank = FrontendCIntegerRank::Long;
		else if (long_count == 2) type.integer_rank = FrontendCIntegerRank::LongLong;
		else type.integer_rank = FrontendCIntegerRank::Int;
		return true;
	}

	bool PrepareDeclaration(uni::Nchain& syntax_tree, uni::Nnode* statement,
		const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
		FrontendCTypeInfo& type_info, bool global = false) {
		uni::Nnode* declarator = statement ? statement->subf : nullptr;
		FrontendCType type;
		if (!ParseTypeSpecifiers(syntax_tree, declarator, type, diagnostics,
			target, type_info)) return false;
		if (!declarator) {
			diagnostics.Report(DiagnosticStage::Parser,
				"expected declarator after type", statement);
			return false;
		}
		if (type.IsVoid()) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"object cannot have void type", statement);
			return false;
		}
		if (global && type.IsRecord()) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"global struct objects are not supported yet", statement);
			return false;
		}
		statement->type = ToToken(global ? FrontendCToken::GlobalDeclaration :
			FrontendCToken::Declaration);
		type_info.Add(statement, type);
		return true;
	}

	bool PrepareFunctionParameters(uni::Nchain& syntax_tree, uni::Nnode* function,
		const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
		FrontendCTypeInfo& type_info) {
		uni::Nnode* node = function ? function->subf : nullptr;
		stduint count = 0;
		while (node) {
			FrontendCType type;
			if (!ParseTypeSpecifiers(syntax_tree, node, type, diagnostics,
				target, type_info)) {
				diagnostics.Report(DiagnosticStage::Parser,
					"expected integer parameter type", node);
				return false;
			}
			while (node && node->type == tok_symbol && node->addr) {
				stduint stars = 0;
				while (node->addr[stars] == '*') stars++;
				if (!stars || node->addr[stars]) break;
				while (stars--) type = type.AddressType();
				node = syntax_tree.Remove(node);
			}
			if (type.IsVoid()) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"parameter cannot have void type", node ? node : function);
				return false;
			}
			if (type.IsRecord()) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"struct parameters by value are not supported yet", node);
				return false;
			}
			uni::Nnode* parameter = node;
			if (!parameter || parameter->type != tok_identy || !parameter->addr) {
				diagnostics.Report(DiagnosticStage::Parser,
					"expected parameter name", parameter ? parameter : function);
				return false;
			}
			parameter->type = ToToken(FrontendCToken::Parameter);
			type_info.Add(parameter, type);
			if (++count > 8) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"more than 8 function parameters are not supported", parameter);
				return false;
			}

			node = parameter->next;
			if (!node) break;
			if (node->type != tok_symbol || !node->addr || StrCompare(node->addr, ",")) {
				diagnostics.Report(DiagnosticStage::Parser,
					"expected ',' between function parameters", node);
				return false;
			}
			node = syntax_tree.Remove(node);
			if (!node) {
				diagnostics.Report(DiagnosticStage::Parser,
					"expected parameter after ','", parameter);
				return false;
			}
		}
		return true;
	}

	stduint AlignUp(stduint value, stduint alignment) {
		if (!alignment) return value;
		stduint remainder = value % alignment;
		return remainder ? value + alignment - remainder : value;
	}

	bool IsRecordDefinition(const uni::Nnode* statement) {
		const uni::Nnode* keyword = statement ? statement->subf : nullptr;
		const uni::Nnode* tag = keyword ? keyword->next : nullptr;
		const uni::Nnode* body = tag ? tag->next : nullptr;
		return IsKeyword(keyword, "struct") && tag && tag->type == tok_identy &&
			tag->addr && body && body->type == tok_block && !body->next;
	}

	bool PrepareRecordDefinition(uni::Nchain& syntax_tree, uni::Nnode* statement,
		const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
		FrontendCTypeInfo& type_info) {
		uni::Nnode* tag = statement->subf->next;
		uni::Nnode* body = tag->next;
		if (type_info.FindRecord(tag->addr)) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"redefinition of struct type", tag);
			return false;
		}

		FrontendCRecord record;
		record.tag = tag->addr;
		type_info.records.Append(record);
		stduint record_id = type_info.records.Count();
		stduint size_bits = 0;
		stduint align_bits = 0;
		for (uni::Nnode* member_statement = body->subf;
			member_statement; member_statement = member_statement->next) {
			if (member_statement->type != tok_statement || !member_statement->subf) {
				diagnostics.Report(DiagnosticStage::Parser,
					"expected struct member declaration", member_statement);
				return false;
			}
			uni::Nnode* declarator = member_statement->subf;
			FrontendCType member_type;
			if (!ParseTypeSpecifiers(syntax_tree, declarator, member_type,
				diagnostics, target, type_info)) return false;
			if (!member_type.IsInteger() || !declarator || declarator->next ||
				declarator->type != tok_identy || !declarator->addr) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"only one integer member per declaration is supported", declarator);
				return false;
			}
			FrontendCRecord* current = type_info.GetRecord(record_id);
			for (const FrontendCRecordMember& previous : current->members)
				if (!StrCompare(previous.name.reference(), declarator->addr)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"duplicate struct member", declarator);
					return false;
				}
			stduint member_size = type_info.SizeBits(member_type, target);
			stduint member_align = type_info.AlignBits(member_type, target);
			if (!member_size || !member_align) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"invalid struct member type", declarator);
				return false;
			}
			size_bits = AlignUp(size_bits, member_align);
			FrontendCRecordMember member;
			member.name = declarator->addr;
			member.type = member_type;
			member.offset_bits = size_bits;
			current->members.Append(member);
			size_bits += member_size;
			if (member_align > align_bits) align_bits = member_align;
		}
		if (!align_bits) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"empty struct is not supported in C99", body);
			return false;
		}
		FrontendCRecord* completed = type_info.GetRecord(record_id);
		completed->align_bits = align_bits;
		completed->size_bits = AlignUp(size_bits, align_bits);
		completed->complete = true;
		statement->type = ToToken(FrontendCToken::RecordDefinition);
		return true;
	}

	void MarkPrefixKeywords(uni::Nnode* node) {
		for (; node; node = node->next) {
			if (node->type == tok_identy && IsKeyword(node, "sizeof"))
				node->type = tok_symbol;
			if (node->subf) MarkPrefixKeywords(node->subf);
		}
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
		const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
		FrontendCTypeInfo& type_info,
		FrontendCDebugInfo* debug_info) {
		if (!block || block->type != tok_block) {
			diagnostics.Report(DiagnosticStage::Parser,
				"expected function block", block);
			return false;
		}

		for (uni::Nnode* node = block->subf; node; node = node->next) {
			if (node->type == tok_block) {
				if (!PrepareBlock(syntax_tree, node, diagnostics, target, type_info, debug_info)) return false;
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
			if (IsRecordDefinition(node)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"block-scope struct definitions are not supported yet", node);
				return false;
			}
			if (IsTypeSpecifier(node->subf)) {
				if (!PrepareDeclaration(syntax_tree, node, diagnostics, target, type_info)) return false;
				MarkPrefixKeywords(node->subf);
				continue;
			}
			MarkPrefixKeywords(node->subf);
			for (uni::Nnode* child = node->subf; child; child = child->next)
				if (child->type == tok_block) {
					if (!PrepareBlock(syntax_tree, child, diagnostics, target, type_info, debug_info)) return false;
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
					node->type = ToToken(FrontendCToken::ReturnVoid);
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
			else if (node->type == ToToken(FrontendCToken::ReturnVoid)) continue;
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

void FrontendCTypeInfo::Add(const uni::Nnode* node, const FrontendCType& type) {
	FrontendCTypedNode typed_node;
	typed_node.node = node;
	typed_node.type = type;
	nodes.Append(typed_node);
}

const FrontendCType* FrontendCTypeInfo::Find(const uni::Nnode* node) const {
	for (const FrontendCTypedNode& typed_node : nodes)
		if (typed_node.node == node) return &typed_node.type;
	return nullptr;
}

stduint FrontendCTypeInfo::FindRecord(const char* tag) const {
	if (!tag) return 0;
	for (stduint index = 0; index < records.Count(); index++)
		if (!StrCompare(records.begin()[index].tag.reference(), tag)) return index + 1;
	return 0;
}

const FrontendCRecord* FrontendCTypeInfo::GetRecord(stduint record_id) const {
	return record_id && record_id <= records.Count() ?
		&records.begin()[record_id - 1] : nullptr;
}

FrontendCRecord* FrontendCTypeInfo::GetRecord(stduint record_id) {
	return record_id && record_id <= records.Count() ?
		&records.begin()[record_id - 1] : nullptr;
}

stduint FrontendCTypeInfo::SizeBits(const FrontendCType& type,
	const FrontendCTargetInfo& target) const {
	if (type.IsPointer()) return target.pointer_bits;
	stduint bits = 0;
	if (type.record_id) {
		const FrontendCRecord* record = GetRecord(type.record_id);
		if (!record || !record->complete) return 0;
		bits = record->size_bits;
	}
	else {
		switch (type.integer_rank) {
		case FrontendCIntegerRank::Char: bits = target.char_bits; break;
		case FrontendCIntegerRank::Short: bits = target.short_bits; break;
		case FrontendCIntegerRank::Int: bits = target.int_bits; break;
		case FrontendCIntegerRank::Long: bits = target.long_bits; break;
		case FrontendCIntegerRank::LongLong: bits = target.long_long_bits; break;
		}
	}
	return bits * type.StorageElementCount();
}

stduint FrontendCTypeInfo::AlignBits(const FrontendCType& type,
	const FrontendCTargetInfo& target) const {
	if (type.IsPointer()) return target.pointer_align_bits;
	if (type.record_id) {
		const FrontendCRecord* record = GetRecord(type.record_id);
		return record && record->complete ? record->align_bits : 0;
	}
	switch (type.integer_rank) {
	case FrontendCIntegerRank::Char: return target.char_align_bits;
	case FrontendCIntegerRank::Short: return target.short_align_bits;
	case FrontendCIntegerRank::Int: return target.int_align_bits;
	case FrontendCIntegerRank::Long: return target.long_align_bits;
	case FrontendCIntegerRank::LongLong: return target.long_long_align_bits;
	}
	return 0;
}

bool frontend_c_detail::Prepare(uni::Nchain& syntax_tree,
	const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
	FrontendCTypeInfo& type_info, FrontendCDebugInfo* debug_info) {
	for (uni::Nnode* node = syntax_tree.Root(); node;) {
		if (node->type == tok_statement) {
			uni::Nnode* next = node->next;
			if (IsRecordDefinition(node)) {
				if (!PrepareRecordDefinition(syntax_tree, node, diagnostics,
					target, type_info)) return false;
				node = next;
				continue;
			}
			if (!node->subf || !IsTypeSpecifier(node->subf)) {
				diagnostics.Report(DiagnosticStage::Parser,
					"expected global declaration", node);
				return false;
			}
			if (!PrepareDeclaration(syntax_tree, node, diagnostics, target,
				type_info, true)) return false;
			MarkPrefixKeywords(node->subf);
			node = next;
			continue;
		}
		FrontendCType return_type;
		if (!ParseTypeSpecifiers(syntax_tree, node, return_type, diagnostics,
			target, type_info)) {
			diagnostics.Report(DiagnosticStage::Parser,
				"expected function return type", node);
			return false;
		}
		if (return_type.IsRecord()) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"struct return values are not supported yet", node);
			return false;
		}
		uni::Nnode* function = node;
		if (!function || function->type != tok_func || !function->addr) {
			diagnostics.Report(DiagnosticStage::Parser,
				"expected function definition", function);
			return false;
		}
		type_info.Add(function, return_type);
		if (!PrepareFunctionParameters(syntax_tree, function, diagnostics, target, type_info)) return false;

		uni::Nnode* body = function->next;
		if (!body || body->type != tok_block) {
			diagnostics.Report(DiagnosticStage::Parser,
				"expected function body", body ? body : function);
			return false;
		}

		function->type = ToToken(FrontendCToken::Function);
		if (!PrepareBlock(syntax_tree, body, diagnostics, target, type_info, debug_info)) return false;
		node = body->next;
	}
	return true;
}

bool frontend_c_detail::Validate(uni::Nchain& syntax_tree,
	const FrontendCDiagnostics& diagnostics) {
	for (uni::Nnode* node = syntax_tree.Root(); node;) {
		if (node->type == ToToken(FrontendCToken::GlobalDeclaration) ||
			node->type == ToToken(FrontendCToken::RecordDefinition)) {
			node = node->next;
			continue;
		}
		uni::Nnode* body = node->next;
		if (node->type != ToToken(FrontendCToken::Function) ||
			!body || body->type != tok_block ||
			!ValidateBlock(body, diagnostics)) return false;
		node = body->next;
	}
	return true;
}

}


