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
			if (node->type == tok_string ||
				node->type == ToToken(FrontendCToken::CharacterLiteral)) {
				char quote = node->type == tok_string ? '"' : '\'';
				text << quote;
				if (node->addr) {
					const char* source = node->addr;
					while (*source) text << *source++;
				}
				text << quote;
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

	const uni::Nnode* ParentNode(const uni::Nnode* node) {
		return node ? const_cast<uni::Nnode*>(node)->getParent() : nullptr;
	}

	const uni::Nnode* EnclosingBlock(const uni::Nnode* node) {
		for (const uni::Nnode* current = node; current; current = ParentNode(current))
			if (current->type == tok_block) return current;
		return nullptr;
	}

	bool ScopeContains(const uni::Nnode* outer, const uni::Nnode* inner) {
		if (!outer) return true;
		for (const uni::Nnode* scope = inner; scope;
			scope = EnclosingBlock(ParentNode(scope)))
			if (scope == outer) return true;
		return false;
	}

	bool NodePrecedes(const uni::Nnode* declaration, const uni::Nnode* use) {
		if (!declaration || !use) return true;
		const uni::TnodeField* left =
			const_cast<uni::Nnode*>(declaration)->GetTnodeField();
		const uni::TnodeField* right =
			const_cast<uni::Nnode*>(use)->GetTnodeField();
		return left->row < right->row ||
			(left->row == right->row && left->col <= right->col);
	}

	bool IsTypeSpecifier(const uni::Nnode* node) {
		if (!node || node->type != tok_identy || !node->addr) return false;
		return IsKeyword(node, "const") ||
			IsKeyword(node, "signed") || IsKeyword(node, "unsigned") ||
			IsKeyword(node, "_Bool") ||
			IsKeyword(node, "char") || IsKeyword(node, "short") ||
			IsKeyword(node, "int") || IsKeyword(node, "long") ||
			IsKeyword(node, "void") || IsKeyword(node, "struct") ||
			IsKeyword(node, "union") || IsKeyword(node, "enum");
	}

	// Consume one C99 scalar type-specifier sequence before its declarator.
	bool ParseTypeSpecifiers(uni::Nchain& syntax_tree, uni::Nnode*& node,
		FrontendCType& type, const FrontendCDiagnostics& diagnostics,
		const FrontendCTargetInfo& target, FrontendCTypeInfo& type_info) {
		unsigned signed_count = 0, unsigned_count = 0, bool_count = 0, char_count = 0;
		unsigned short_count = 0, int_count = 0, long_count = 0, void_count = 0;
		unsigned const_count = 0;
		uni::Nnode* first = node;
		uni::Nnode* after_type = node;
		while (IsKeyword(after_type, "const")) {
			const_count++;
			after_type = after_type->next;
		}
		bool is_union = IsKeyword(after_type, "union");
		bool is_enum = IsKeyword(after_type, "enum");
		if (IsKeyword(after_type, "struct") || is_union || is_enum) {
			uni::Nnode* tag = after_type->next;
			if (const_count > 1 || !tag || tag->type != tok_identy || !tag->addr) {
				diagnostics.Report(DiagnosticStage::Parser,
					is_enum ? "expected enum tag" : "expected record tag", after_type);
				return false;
			}
			stduint type_id = is_enum ? type_info.FindEnum(tag->addr, tag) :
				type_info.FindRecord(tag->addr);
			if (is_enum) {
				if (!type_id) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"use of undefined enum type", tag);
					return false;
				}
			}
			else {
				if (type_info.FindEnum(tag->addr, tag)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"record tag is hidden by an enum tag", tag);
					return false;
				}
				const FrontendCRecord* record = type_info.GetRecord(type_id);
				if (!record || record->is_union != is_union) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"use of undefined or mismatched record type", tag);
					return false;
				}
			}
			after_type = tag->next;
			while (node != after_type) node = syntax_tree.Remove(node);
			type = FrontendCType();
			if (is_enum) {
				type.enum_id = type_id;
				const FrontendCEnum* enumeration = type_info.GetEnum(type_id);
				type.is_incomplete = !enumeration || !enumeration->body;
			}
			else type.record_id = type_id;
			type.base_qualifiers.is_const = const_count != 0;
			return true;
		}
		after_type = first;
		const_count = 0;
		while (IsTypeSpecifier(after_type) && !IsKeyword(after_type, "struct") &&
			!IsKeyword(after_type, "union")) {
			if (IsKeyword(after_type, "const")) const_count++;
			else if (IsKeyword(after_type, "signed")) signed_count++;
			else if (IsKeyword(after_type, "unsigned")) unsigned_count++;
			else if (IsKeyword(after_type, "_Bool")) bool_count++;
			else if (IsKeyword(after_type, "char")) char_count++;
			else if (IsKeyword(after_type, "short")) short_count++;
			else if (IsKeyword(after_type, "int")) int_count++;
			else if (IsKeyword(after_type, "long")) long_count++;
			else if (IsKeyword(after_type, "void")) void_count++;
			after_type = after_type->next;
		}
		if (after_type == first) return false;
		if (const_count > 1 || signed_count > 1 || unsigned_count > 1 || void_count > 1 ||
			!(signed_count || unsigned_count || bool_count || char_count || short_count ||
				int_count || long_count || void_count) ||
			(signed_count && unsigned_count) || char_count > 1 ||
			bool_count > 1 || short_count > 1 || int_count > 1 || long_count > 2 ||
			(bool_count && (signed_count || unsigned_count || char_count ||
				short_count || int_count || long_count || void_count)) ||
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
		if (bool_count) {
			type.integer_rank = FrontendCIntegerRank::Bool;
			type.is_signed = false;
		}
		else if (char_count) type.integer_rank = FrontendCIntegerRank::Char;
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
					"record parameters by value are not supported yet", node);
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

	constexpr stduint MaximumExplicitRecordAlignmentBytes = 16;

	bool IsRecordLayoutModifier(const uni::Nnode* node) {
		return IsKeyword(node, "packed") || IsKeyword(node, "padded") ||
			(node && node->type == tok_func &&
				(IsKeyword(node, "alignb") || IsKeyword(node, "aligne")));
	}

	const uni::Nnode* SkipRecordLayoutModifiers(const uni::Nnode* node) {
		while (IsRecordLayoutModifier(node)) node = node->next;
		return node;
	}

	bool IsRecordDefinition(const uni::Nnode* statement) {
		const uni::Nnode* keyword = SkipRecordLayoutModifiers(
			statement ? statement->subf : nullptr);
		const uni::Nnode* tag = keyword ? keyword->next : nullptr;
		const uni::Nnode* body = tag ? tag->next : nullptr;
		return (IsKeyword(keyword, "struct") || IsKeyword(keyword, "union")) &&
			tag && tag->type == tok_identy && tag->addr && body &&
			body->type == tok_block && !body->next;
	}

	bool IsRecordDeclaration(const uni::Nnode* statement) {
		const uni::Nnode* keyword = statement ? statement->subf : nullptr;
		const uni::Nnode* tag = keyword ? keyword->next : nullptr;
		return (IsKeyword(keyword, "struct") || IsKeyword(keyword, "union")) &&
			tag && tag->type == tok_identy && tag->addr && !tag->next;
	}

	bool IsEnumDefinition(const uni::Nnode* statement) {
		const uni::Nnode* keyword = statement ? statement->subf : nullptr;
		const uni::Nnode* tag = keyword ? keyword->next : nullptr;
		const uni::Nnode* body = tag ? tag->next : nullptr;
		return IsKeyword(keyword, "enum") && tag && tag->type == tok_identy &&
			tag->addr && body && body->type == tok_block;
	}

	bool IsEnumForwardDeclaration(const uni::Nnode* statement) {
		const uni::Nnode* keyword = statement ? statement->subf : nullptr;
		const uni::Nnode* tag = keyword ? keyword->next : nullptr;
		return IsKeyword(keyword, "enum") && tag && tag->type == tok_identy &&
			tag->addr && !tag->next;
	}

	bool PrepareEnumStatement(uni::Nchain& syntax_tree, uni::Nnode* statement,
		const uni::Nnode* scope, bool global,
		const FrontendCDiagnostics& diagnostics, FrontendCTypeInfo& type_info) {
		uni::Nnode* keyword = statement ? statement->subf : nullptr;
		uni::Nnode* tag = keyword ? keyword->next : nullptr;
		if (!tag || !tag->addr) return false;
		uni::Nnode* body = tag->next && tag->next->type == tok_block ?
			tag->next : nullptr;
		stduint enum_id = type_info.FindEnumInScope(tag->addr, scope);
		FrontendCEnum* enumeration = type_info.GetEnum(enum_id);
		if (!scope && type_info.FindRecord(tag->addr)) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"enum tag conflicts with record tag", tag);
			return false;
		}
		if (body && enumeration && enumeration->body) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"redefinition of enum tag", tag);
			return false;
		}
		if (!enumeration) {
			FrontendCEnum declaration;
			declaration.tag = tag->addr;
			declaration.scope = scope;
			declaration.tag_declaration = statement;
			type_info.enums.Append(declaration);
			enum_id = type_info.enums.Count();
			enumeration = type_info.GetEnum(enum_id);
		}
		if (!body) {
			statement->type = ToToken(FrontendCToken::EnumDeclaration);
			if (!enumeration->declaration) enumeration->declaration = statement;
			return true;
		}

		uni::Nnode* marker = statement;
		if (body->next) {
			uni::Nnode* declarator = body->next;
			uni::Nnode* declaration = syntax_tree.Append(nullptr, false, statement);
			*declaration->GetTnodeField() = *statement->GetTnodeField();
			syntax_tree.Adopt(declaration, declarator, declarator->Tail(), false);
			declaration->type = ToToken(global ? FrontendCToken::GlobalDeclaration :
				FrontendCToken::Declaration);
			FrontendCType type;
			type.enum_id = enum_id;
			type_info.Add(declaration, type);
		}
		marker->type = ToToken(FrontendCToken::EnumDefinition);
		enumeration->declaration = marker;
		enumeration->body = body;
		return true;
	}

	bool ParseRecordAlignment(const uni::Nnode* modifier,
		const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
		stduint& align_bits) {
		const uni::Nnode* argument = modifier ? modifier->subf : nullptr;
		if (!argument || argument->next || argument->type != tok_number ||
			!argument->addr) {
			diagnostics.Report(DiagnosticStage::Parser,
				"record alignment requires one integer argument", modifier);
			return false;
		}
		stdsint value;
		FrontendCType literal_type;
		if (!frontend_c_detail::ParseIntegerLiteral(
			argument->addr, target, value, literal_type)) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"record alignment requires a valid integer constant", argument);
			return false;
		}
		stduint align_bytes = 0;
		if (IsKeyword(modifier, "alignb")) {
			if (value <= 0) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"alignb requires a positive byte count", argument);
				return false;
			}
			align_bytes = (stduint)value;
		}
		else {
			if (value < 0 || value > 4) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"aligne exponent must be between 0 and 4", argument);
				return false;
			}
			align_bytes = stduint(1) << value;
		}
		if ((align_bytes & (align_bytes - 1)) ||
			align_bytes > MaximumExplicitRecordAlignmentBytes) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"record alignment must be a power of two up to 16 bytes", argument);
			return false;
		}
		align_bits = align_bytes * target.char_bits;
		return true;
	}

	bool ParseRecordDefinitionPrefix(uni::Nnode* statement,
		const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
		uni::Nnode*& keyword, FrontendCRecordLayout& layout,
		stduint& explicit_align_bits) {
		keyword = statement ? statement->subf : nullptr;
		layout = FrontendCRecordLayout::Default;
		explicit_align_bits = 0;
		while (IsRecordLayoutModifier(keyword)) {
			if (IsKeyword(keyword, "packed") || IsKeyword(keyword, "padded")) {
				if (layout != FrontendCRecordLayout::Default) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"conflicting or duplicate record layout modifier", keyword);
					return false;
				}
				layout = IsKeyword(keyword, "packed") ?
					FrontendCRecordLayout::Packed : FrontendCRecordLayout::Padded;
			}
			else {
				if (explicit_align_bits) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"duplicate record alignment modifier", keyword);
					return false;
				}
				if (!ParseRecordAlignment(keyword, diagnostics, target,
					explicit_align_bits)) return false;
			}
			keyword = keyword->next;
		}
		return true;
	}

	bool PrepareRecordDeclaration(uni::Nnode* statement,
		const FrontendCDiagnostics& diagnostics, FrontendCTypeInfo& type_info) {
		uni::Nnode* keyword = statement->subf;
		uni::Nnode* tag = keyword->next;
		bool is_union = IsKeyword(keyword, "union");
		if (type_info.FindEnum(tag->addr)) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"record tag conflicts with enum tag", tag);
			return false;
		}
		stduint record_id = type_info.FindRecord(tag->addr);
		FrontendCRecord* record = type_info.GetRecord(record_id);
		if (record && record->is_union != is_union) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"record tag redeclared with a different kind", tag);
			return false;
		}
		if (!record) {
			FrontendCRecord declaration;
			declaration.tag = tag->addr;
			declaration.is_union = is_union;
			type_info.records.Append(declaration);
		}
		statement->type = ToToken(FrontendCToken::RecordDeclaration);
		return true;
	}

	// Parse the identifier and array dimensions of one record member declarator.
	bool ParseRecordMemberDeclarator(uni::Nnode* node, uni::Nnode*& identifier,
		FrontendCType& type, const FrontendCTargetInfo& target) {
		if (!node) return false;
		if (node->type == tok_func && node->addr &&
			!StrCompare(node->addr, "OP@PREMEMB") && node->subf &&
			!node->subf->next) {
			if (!ParseRecordMemberDeclarator(
				node->subf, identifier, type, target)) return false;
			type = type.AddressType();
			return true;
		}
		if (node->type == tok_func && node->addr &&
			!StrCompare(node->addr, "operator[]")) {
			uni::Nnode* declarator = node->subf;
			uni::Nnode* dimension = declarator ? declarator->next : nullptr;
			if (!declarator || !dimension || dimension->next ||
				dimension->type != tok_number || !dimension->addr ||
				!ParseRecordMemberDeclarator(
					declarator, identifier, type, target)) return false;
			stdsint count;
			FrontendCType literal_type;
			return frontend_c_detail::ParseIntegerLiteral(
				dimension->addr, target, count, literal_type) && count > 0 &&
				type.AppendArrayDimension((stduint)count);
		}
		if (node->type != tok_identy || !node->addr) return false;
		identifier = node;
		return true;
	}

	bool PrepareRecordDefinition(uni::Nchain& syntax_tree, uni::Nnode* statement,
		const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
		FrontendCTypeInfo& type_info) {
		uni::Nnode* keyword;
		FrontendCRecordLayout layout;
		stduint explicit_align_bits;
		if (!ParseRecordDefinitionPrefix(statement, diagnostics, target,
			keyword, layout, explicit_align_bits)) return false;
		uni::Nnode* tag = keyword->next;
		uni::Nnode* body = tag->next;
		bool is_union = IsKeyword(keyword, "union");
		if (type_info.FindEnum(tag->addr)) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"record tag conflicts with enum tag", tag);
			return false;
		}
		stduint record_id = type_info.FindRecord(tag->addr);
		FrontendCRecord* record = type_info.GetRecord(record_id);
		if (record && record->is_union != is_union) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"record tag defined with a different kind", tag);
			return false;
		}
		if (record && record->complete) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"redefinition of record type", tag);
			return false;
		}
		if (!record) {
			FrontendCRecord definition;
			definition.tag = tag->addr;
			definition.is_union = is_union;
			type_info.records.Append(definition);
			record_id = type_info.records.Count();
			record = type_info.GetRecord(record_id);
		}
		record->layout = layout;
		record->explicit_align_bits = explicit_align_bits;
		stduint size_bits = 0;
		stduint align_bits = 0;
		for (uni::Nnode* member_statement = body->subf;
			member_statement; member_statement = member_statement->next) {
			if (member_statement->type != tok_statement || !member_statement->subf) {
				diagnostics.Report(DiagnosticStage::Parser,
					"expected record member declaration", member_statement);
				return false;
			}
			uni::Nnode* declarator = member_statement->subf;
			FrontendCType member_type;
			if (!ParseTypeSpecifiers(syntax_tree, declarator, member_type,
				diagnostics, target, type_info)) return false;
			while (declarator && declarator->type == tok_symbol && declarator->addr) {
				stduint stars = 0;
				while (declarator->addr[stars] == '*') stars++;
				if (!stars || declarator->addr[stars]) break;
				while (stars--) member_type = member_type.AddressType();
				declarator = syntax_tree.Remove(declarator);
			}
			uni::Nnode* identifier = nullptr;
			if (!declarator || declarator->next || member_type.IsVoid() ||
				!ParseRecordMemberDeclarator(
					declarator, identifier, member_type, target)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"unsupported record member declaration", declarator);
				return false;
			}
			FrontendCRecord* current = type_info.GetRecord(record_id);
			for (const FrontendCRecordMember& previous : current->members)
				if (!StrCompare(previous.name.reference(), identifier->addr)) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"duplicate record member", identifier);
					return false;
				}
			stduint member_size = type_info.SizeBits(member_type, target);
			stduint member_align = type_info.AlignBits(member_type, target);
			if (!member_size || !member_align) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"record member has incomplete type", identifier);
				return false;
			}
			bool is_packed = layout == FrontendCRecordLayout::Packed;
			if (!is_union && !is_packed) size_bits = AlignUp(size_bits, member_align);
			FrontendCRecordMember member;
			member.name = identifier->addr;
			member.type = member_type;
			member.offset_bits = is_union ? 0 : size_bits;
			current->members.Append(member);
			if (is_union) {
				if (member_size > size_bits) size_bits = member_size;
			}
			else size_bits += member_size;
			if (!is_packed && member_align > align_bits) align_bits = member_align;
		}
		bool is_packed = layout == FrontendCRecordLayout::Packed;
		if (!is_packed && !align_bits) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"empty record is not supported in C99", body);
			return false;
		}
		if (is_packed && !size_bits) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"empty packed record is not supported", body);
			return false;
		}
		FrontendCRecord* completed = type_info.GetRecord(record_id);
		stduint natural_align_bits = is_packed ? target.char_align_bits : align_bits;
		if (explicit_align_bits && !is_packed &&
			explicit_align_bits < natural_align_bits) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"explicit alignment cannot reduce padded ABI alignment", statement);
			return false;
		}
		completed->align_bits = explicit_align_bits ?
			explicit_align_bits : natural_align_bits;
		completed->size_bits = AlignUp(size_bits, completed->align_bits);
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
			if (node->type == ToToken(FrontendCToken::Declaration)) {
				SaveStatementSource(debug_info, node);
				MarkPrefixKeywords(node->subf);
				continue;
			}
			if (node->type == ToToken(FrontendCToken::EnumDeclaration) ||
				node->type == ToToken(FrontendCToken::EnumDefinition)) continue;
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
			if (IsEnumDefinition(node) || IsEnumForwardDeclaration(node)) {
				if (!PrepareEnumStatement(syntax_tree, node, block, false,
					diagnostics, type_info)) return false;
				continue;
			}
			if (IsRecordDefinition(node)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"block-scope record definitions are not supported yet", node);
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
				node->type == ToToken(FrontendCToken::EnumDeclaration) ||
				node->type == ToToken(FrontendCToken::EnumDefinition) ||
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

// Parse the shared type-name form used by sizeof and explicit casts.
bool frontend_c_detail::ParseTypeName(const uni::Nnode* node,
	FrontendCType& type, const FrontendCTargetInfo& target,
	const FrontendCTypeInfo& type_info, const uni::Nnode* end) {
	if (!end && node && node->type == tok_func && node->addr &&
		!StrCompare(node->addr, "operator[]")) {
		const uni::Nnode* dimension = node->subf ? node->subf->Tail() : nullptr;
		if (!dimension || dimension == node->subf || dimension->type != tok_number ||
			!dimension->addr || !ParseTypeName(node->subf, type,
				target, type_info, dimension)) return false;
		stdsint count;
		FrontendCType literal_type;
		return frontend_c_detail::ParseIntegerLiteral(
			dimension->addr, target, count, literal_type) && count > 0 &&
			type.AppendArrayDimension((stduint)count);
	}
	const uni::Nnode* current = node;
	unsigned const_count = 0;
	while (current != end && IsKeyword(current, "const")) {
		const_count++;
		current = current->next;
	}

	bool is_union = IsKeyword(current, "union");
	bool is_enum = IsKeyword(current, "enum");
	if (IsKeyword(current, "struct") || is_union || is_enum) {
		const uni::Nnode* tag = current->next;
		if (!is_enum && tag && tag != end && tag->next == end && tag->type == tok_func &&
			tag->addr && !StrCompare(tag->addr, "operator[]")) {
			const uni::Nnode* tag_name = tag->subf;
			const uni::Nnode* dimension = tag_name ? tag_name->next : nullptr;
			if (const_count > 1 || !tag_name || !dimension || dimension->next ||
				tag_name->type != tok_identy || !tag_name->addr ||
				dimension->type != tok_number || !dimension->addr) return false;
			stduint record_id = type_info.FindRecord(tag_name->addr);
			const FrontendCRecord* record = type_info.GetRecord(record_id);
			stdsint count;
			FrontendCType literal_type;
			if (!record || record->is_union != is_union ||
				!frontend_c_detail::ParseIntegerLiteral(
					dimension->addr, target, count, literal_type) || count <= 0) return false;
			type = FrontendCType();
			type.record_id = record_id;
			type.base_qualifiers.is_const = const_count != 0;
			return type.AppendArrayDimension((stduint)count);
		}
		if (const_count > 1 || !tag || tag == end ||
			tag->type != tok_identy || !tag->addr) return false;
		type = FrontendCType();
		if (is_enum) {
			type.enum_id = type_info.FindEnum(tag->addr, tag);
			if (!type.enum_id) return false;
			const FrontendCEnum* enumeration = type_info.GetEnum(type.enum_id);
			type.is_incomplete = !enumeration || !enumeration->body ||
				!NodePrecedes(enumeration->declaration, tag);
			if (enumeration && enumeration->complete) {
				type.integer_rank = enumeration->integer_rank;
				type.is_signed = enumeration->is_signed;
			}
		}
		else {
			if (type_info.FindEnum(tag->addr, tag)) return false;
			type.record_id = type_info.FindRecord(tag->addr);
			const FrontendCRecord* record = type_info.GetRecord(type.record_id);
			if (!record || record->is_union != is_union) return false;
		}
		type.base_qualifiers.is_const = const_count != 0;
		current = tag->next;
		while (current != end && IsKeyword(current, "const")) {
			if (type.base_qualifiers.is_const) return false;
			type.base_qualifiers.is_const = true;
			current = current->next;
		}
	}
	else {
		unsigned signed_count = 0, unsigned_count = 0, bool_count = 0, char_count = 0;
		unsigned short_count = 0, int_count = 0, long_count = 0, void_count = 0;
		bool found_specifier = false;
		const_count = 0;
		current = node;
		for (; current != end && current && current->type == tok_identy;
			current = current->next) {
			if (IsKeyword(current, "const")) const_count++;
			else if (IsKeyword(current, "signed")) signed_count++;
			else if (IsKeyword(current, "unsigned")) unsigned_count++;
			else if (IsKeyword(current, "_Bool")) bool_count++;
			else if (IsKeyword(current, "char")) char_count++;
			else if (IsKeyword(current, "short")) short_count++;
			else if (IsKeyword(current, "int")) int_count++;
			else if (IsKeyword(current, "long")) long_count++;
			else if (IsKeyword(current, "void")) void_count++;
			else break;
			if (!IsKeyword(current, "const")) found_specifier = true;
		}
		if (!found_specifier || const_count > 1 || signed_count > 1 ||
			unsigned_count > 1 || void_count > 1 ||
			(signed_count && unsigned_count) || char_count > 1 ||
			bool_count > 1 || short_count > 1 || int_count > 1 || long_count > 2 ||
			(bool_count && (signed_count || unsigned_count || char_count ||
				short_count || int_count || long_count || void_count)) ||
			(char_count && (short_count || int_count || long_count)) ||
			(short_count && long_count) ||
			(void_count && (signed_count || unsigned_count || char_count ||
				short_count || int_count || long_count))) return false;
		type = FrontendCType();
		type.is_void = void_count != 0;
		type.base_qualifiers.is_const = const_count != 0;
		type.is_signed = unsigned_count ? false :
			(char_count && !signed_count ? target.char_is_signed : true);
		if (bool_count) {
			type.integer_rank = FrontendCIntegerRank::Bool;
			type.is_signed = false;
		}
		else if (char_count) type.integer_rank = FrontendCIntegerRank::Char;
		else if (short_count) type.integer_rank = FrontendCIntegerRank::Short;
		else if (long_count == 1) type.integer_rank = FrontendCIntegerRank::Long;
		else if (long_count == 2) type.integer_rank = FrontendCIntegerRank::LongLong;
	}

	while (current != end && current && current->type == tok_symbol &&
		current->addr) {
		stduint stars = 0;
		while (current->addr[stars] == '*') stars++;
		if (!stars || current->addr[stars]) break;
		while (stars--) {
			if (type.pointer_depth >= FrontendCType::MaxPointerDepth) return false;
			type = type.AddressType();
		}
		current = current->next;
		if (current != end && IsKeyword(current, "const")) {
			type.pointer_qualifiers[type.pointer_depth - 1].is_const = true;
			current = current->next;
			if (current != end && IsKeyword(current, "const")) return false;
		}
	}
	return current == end;
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

stduint FrontendCTypeInfo::FindEnumInScope(
	const char* tag, const uni::Nnode* scope) const {
	if (!tag) return 0;
	for (stduint index = enums.Count(); index; index--) {
		const FrontendCEnum& enumeration = enums.begin()[index - 1];
		if (enumeration.scope == scope &&
			!StrCompare(enumeration.tag.reference(), tag)) return index;
	}
	return 0;
}

stduint FrontendCTypeInfo::FindEnum(
	const char* tag, const uni::Nnode* use) const {
	if (!tag) return 0;
	const uni::Nnode* use_scope = EnclosingBlock(use);
	for (stduint index = enums.Count(); index; index--) {
		const FrontendCEnum& enumeration = enums.begin()[index - 1];
		if (StrCompare(enumeration.tag.reference(), tag) ||
			!ScopeContains(enumeration.scope, use_scope) ||
			!NodePrecedes(enumeration.tag_declaration, use)) continue;
		return index;
	}
	return 0;
}

const FrontendCEnum* FrontendCTypeInfo::GetEnum(stduint enum_id) const {
	return enum_id && enum_id <= enums.Count() ?
		&enums.begin()[enum_id - 1] : nullptr;
}

FrontendCEnum* FrontendCTypeInfo::GetEnum(stduint enum_id) {
	return enum_id && enum_id <= enums.Count() ?
		&enums.begin()[enum_id - 1] : nullptr;
}

const FrontendCEnumerator* FrontendCTypeInfo::FindEnumerator(
	const char* name, const uni::Nnode* use) const {
	if (!name) return nullptr;
	const uni::Nnode* use_scope = EnclosingBlock(use);
	for (stduint enum_index = enums.Count(); enum_index; enum_index--) {
		const FrontendCEnum& enumeration = enums.begin()[enum_index - 1];
		if (!ScopeContains(enumeration.scope, use_scope)) continue;
		for (stduint item_index = enumeration.enumerators.Count(); item_index; item_index--) {
			const FrontendCEnumerator& enumerator =
				enumeration.enumerators.begin()[item_index - 1];
			if (!StrCompare(enumerator.name.reference(), name) &&
				NodePrecedes(enumerator.declaration, use)) return &enumerator;
		}
	}
	return nullptr;
}

stduint FrontendCTypeInfo::SizeBits(const FrontendCType& type,
	const FrontendCTargetInfo& target) const {
	if (type.IsPointer()) return target.pointer_bits;
	if (type.enum_id && type.is_incomplete) return 0;
	stduint bits = 0;
	if (type.record_id) {
		const FrontendCRecord* record = GetRecord(type.record_id);
		if (!record || !record->complete) return 0;
		bits = record->size_bits;
	}
	else {
		FrontendCIntegerRank rank = type.integer_rank;
		if (type.enum_id) {
			const FrontendCEnum* enumeration = GetEnum(type.enum_id);
			if (!enumeration || !enumeration->complete) return 0;
			rank = enumeration->integer_rank;
		}
		switch (rank) {
		case FrontendCIntegerRank::Bool: bits = target.char_bits; break;
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
	if (type.enum_id && type.is_incomplete) return 0;
	if (type.record_id) {
		const FrontendCRecord* record = GetRecord(type.record_id);
		return record && record->complete ? record->align_bits : 0;
	}
	FrontendCIntegerRank rank = type.integer_rank;
	if (type.enum_id) {
		const FrontendCEnum* enumeration = GetEnum(type.enum_id);
		if (!enumeration || !enumeration->complete) return 0;
		rank = enumeration->integer_rank;
	}
	switch (rank) {
	case FrontendCIntegerRank::Bool: return target.char_align_bits;
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
			if (IsEnumDefinition(node) || IsEnumForwardDeclaration(node)) {
				if (!PrepareEnumStatement(syntax_tree, node, nullptr, true,
					diagnostics, type_info)) return false;
				if (node->next &&
					node->next->type == ToToken(FrontendCToken::GlobalDeclaration))
					MarkPrefixKeywords(node->next->subf);
				node = next;
				continue;
			}
			if (IsRecordDeclaration(node)) {
				if (!PrepareRecordDeclaration(node, diagnostics, type_info)) return false;
				node = next;
				continue;
			}
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
				"record return values are not supported yet", node);
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

namespace {

	void AppendEnumeratorNodes(uni::Nnode* node,
		uni::Vector<uni::Nnode*>& enumerators) {
		if (node && node->type == tok_func && node->addr &&
			!StrCompare(node->addr, "OP@COMMA")) {
			for (uni::Nnode* child = node->subf; child; child = child->next)
				AppendEnumeratorNodes(child, enumerators);
			return;
		}
		if (node) enumerators.Append(node);
	}

	unsigned IntegerRankBits(FrontendCIntegerRank rank,
		const FrontendCTargetInfo& target) {
		switch (rank) {
		case FrontendCIntegerRank::Bool: return 1;
		case FrontendCIntegerRank::Char: return target.char_bits;
		case FrontendCIntegerRank::Short: return target.short_bits;
		case FrontendCIntegerRank::Int: return target.int_bits;
		case FrontendCIntegerRank::Long: return target.long_bits;
		case FrontendCIntegerRank::LongLong: return target.long_long_bits;
		}
		return 0;
	}

	bool FitsSigned(stdsint value, unsigned bits) {
		if (!bits || bits > sizeof(stdsint) * 8) return false;
		if (bits == sizeof(stdsint) * 8) return true;
		stdsint minimum = -(stdsint)(stduint(1) << (bits - 1));
		stdsint maximum = (stdsint)((stduint(1) << (bits - 1)) - 1);
		return value >= minimum && value <= maximum;
	}

	bool FitsUnsigned(stdsint value, unsigned bits) {
		if (value < 0 || !bits || bits > sizeof(stduint) * 8) return false;
		if (bits == sizeof(stduint) * 8) return true;
		return (stduint)value <= ((stduint(1) << bits) - 1);
	}

	stdsint SignedMinimum() {
		return (stdsint)(stduint(1) << (sizeof(stdsint) * 8 - 1));
	}

	stdsint SignedMaximum() {
		return (stdsint)((stduint)-1 >> 1);
	}

	bool CastIntegerValue(stdsint& value, const FrontendCType& type,
		const FrontendCTargetInfo& target) {
		unsigned bits = IntegerRankBits(type.integer_rank, target);
		if (!bits || bits > sizeof(stduint) * 8) return false;
		if (type.integer_rank == FrontendCIntegerRank::Bool) {
			value = value != 0;
			return true;
		}
		stduint pattern = (stduint)value;
		if (bits < sizeof(stduint) * 8)
			pattern &= (stduint(1) << bits) - 1;
		if (type.is_signed && bits < sizeof(stduint) * 8 &&
			(pattern & (stduint(1) << (bits - 1))))
			pattern |= ~((stduint(1) << bits) - 1);
		value = (stdsint)pattern;
		return true;
	}

	bool SelectEnumCompatibleType(stdsint minimum, stdsint maximum,
		const FrontendCTargetInfo& target, FrontendCIntegerRank& rank,
		bool& is_signed) {
		const FrontendCIntegerRank ranks[] = {
			FrontendCIntegerRank::Int,
			FrontendCIntegerRank::Long,
			FrontendCIntegerRank::LongLong
		};
		for (FrontendCIntegerRank candidate : ranks) {
			unsigned bits = IntegerRankBits(candidate, target);
			if (FitsSigned(minimum, bits) && FitsSigned(maximum, bits)) {
				rank = candidate;
				is_signed = true;
				return true;
			}
			if (minimum >= 0 && FitsUnsigned(maximum, bits)) {
				rank = candidate;
				is_signed = false;
				return true;
			}
		}
		return false;
	}

	bool EvaluateEnumeratorValue(uni::Nnode* node,
		const FrontendCTargetInfo& target, const FrontendCTypeInfo& type_info,
		stdsint& value) {
		if (!node) return false;
		if (node->type == tok_number) {
			FrontendCType literal_type;
			if (!frontend_c_detail::ParseIntegerLiteral(
				node->addr, target, value, literal_type)) return false;
			return literal_type.is_signed || value >= 0 ||
				IntegerRankBits(literal_type.integer_rank, target) < 64;
		}
		if (node->type == ToToken(FrontendCToken::CharacterLiteral))
			return frontend_c_detail::ParseCharacterLiteral(node->addr, target, value);
		if (node->type == tok_identy) {
			const FrontendCEnumerator* enumerator =
				type_info.FindEnumerator(node->addr, node);
			if (!enumerator) return false;
			value = enumerator->value;
			return true;
		}
		if (node->type != tok_func || !node->subf) return false;
		if (!node->addr)
			return !node->subf->next && EvaluateEnumeratorValue(
				node->subf, target, type_info, value);
		uni::Nnode* left_node = node->subf;
		uni::Nnode* right_node = left_node->next;
		if (!StrCompare(node->addr, "OP@CAST")) {
			const FrontendCType* cast_type = type_info.Find(node);
			if (right_node || !cast_type || !cast_type->IsInteger() ||
				!EvaluateEnumeratorValue(left_node, target, type_info, value) ||
				!CastIntegerValue(value, *cast_type, target)) return false;
			return cast_type->is_signed || value >= 0 ||
				IntegerRankBits(cast_type->integer_rank, target) < 64;
		}
		if (!StrCompare(node->addr, "OP@SIZEOF") ||
			!StrCompare(node->addr, "sizeof")) {
			FrontendCType operand_type;
			if (right_node || !frontend_c_detail::ParseTypeName(
				left_node, operand_type, target, type_info)) return false;
			stduint bits = type_info.SizeBits(operand_type, target);
			if (!bits || bits % target.char_bits) return false;
			value = (stdsint)(bits / target.char_bits);
			return true;
		}
		if (!right_node) {
			if (!EvaluateEnumeratorValue(left_node, target, type_info, value)) return false;
			if (!StrCompare(node->addr, "OP@PREPOSI")) return true;
			if (!StrCompare(node->addr, "OP@PRENEGA")) {
				if (value == SignedMinimum()) return false;
				value = -value;
				return true;
			}
			if (!StrCompare(node->addr, "OP@BITNOT")) {
				value = ~value;
				return true;
			}
			if (!StrCompare(node->addr, "OP@LOGNOT")) {
				value = !value;
				return true;
			}
			return false;
		}
		if (!StrCompare(node->addr, "OP@LOGAND")) {
			if (!EvaluateEnumeratorValue(left_node, target, type_info, value)) return false;
			if (!value) return true;
			return EvaluateEnumeratorValue(right_node, target, type_info, value) &&
				(value = value != 0, true);
		}
		if (!StrCompare(node->addr, "OP@LOGIOR")) {
			if (!EvaluateEnumeratorValue(left_node, target, type_info, value)) return false;
			if (value) {
				value = 1;
				return true;
			}
			return EvaluateEnumeratorValue(right_node, target, type_info, value) &&
				(value = value != 0, true);
		}
		if (!StrCompare(node->addr, "OP@TERNAR")) {
			uni::Nnode* false_node = right_node->next;
			if (!false_node || false_node->next ||
				!EvaluateEnumeratorValue(left_node, target, type_info, value)) return false;
			return EvaluateEnumeratorValue(value ? right_node : false_node,
				target, type_info, value);
		}
		if (right_node->next) return false;
		stdsint left, right;
		if (!EvaluateEnumeratorValue(left_node, target, type_info, left) ||
			!EvaluateEnumeratorValue(right_node, target, type_info, right)) return false;
		if (!StrCompare(node->addr, "OP@ARIADD"))
			return !__builtin_add_overflow(left, right, &value);
		if (!StrCompare(node->addr, "OP@ARISUB"))
			return !__builtin_sub_overflow(left, right, &value);
		if (!StrCompare(node->addr, "OP@ARIMUL"))
			return !__builtin_mul_overflow(left, right, &value);
		if (!StrCompare(node->addr, "OP@ARIDIV")) {
			if (!right || (left == SignedMinimum() && right == -1)) return false;
			value = left / right;
			return true;
		}
		if (!StrCompare(node->addr, "OP@ARIREM")) {
			if (!right || (left == SignedMinimum() && right == -1)) return false;
			value = left % right;
			return true;
		}
		if (!StrCompare(node->addr, "OP@BITSHL")) {
			if (left < 0 || right < 0 || right >= (stdsint)(sizeof(stdsint) * 8) ||
				left > (SignedMaximum() >> right)) return false;
			value = left << right;
			return true;
		}
		if (!StrCompare(node->addr, "OP@BITSHR")) {
			if (right < 0 || right >= (stdsint)(sizeof(stdsint) * 8)) return false;
			value = left >> right;
			return true;
		}
		if (!StrCompare(node->addr, "OP@BITAND")) value = left & right;
		else if (!StrCompare(node->addr, "OP@BITXOR")) value = left ^ right;
		else if (!StrCompare(node->addr, "OP@BITWOR")) value = left | right;
		else if (!StrCompare(node->addr, "OP@JEQUAL")) value = left == right;
		else if (!StrCompare(node->addr, "OP@JNOTEQ")) value = left != right;
		else if (!StrCompare(node->addr, "OP@JBELOW")) value = left < right;
		else if (!StrCompare(node->addr, "OP@JBEEQU")) value = left <= right;
		else if (!StrCompare(node->addr, "OP@JGREAT")) value = left > right;
		else if (!StrCompare(node->addr, "OP@JGREQU")) value = left >= right;
		else return false;
		return true;
	}

	bool EnumeratorExistsInScope(const FrontendCTypeInfo& type_info,
		const char* name, const uni::Nnode* scope) {
		for (const FrontendCEnum& enumeration : type_info.enums) {
			if (enumeration.scope != scope) continue;
			for (const FrontendCEnumerator& enumerator : enumeration.enumerators)
				if (!StrCompare(enumerator.name.reference(), name)) return true;
		}
		return false;
	}

	void ApplyEnumCompatibleType(FrontendCType& type,
		const FrontendCTypeInfo& type_info) {
		if (!type.enum_id) return;
		const FrontendCEnum* enumeration = type_info.GetEnum(type.enum_id);
		if (!enumeration || !enumeration->complete) return;
		type.integer_rank = enumeration->integer_rank;
		type.is_signed = enumeration->is_signed;
	}

}

bool frontend_c_detail::FinalizeEnums(uni::Nchain& syntax_tree,
	const FrontendCDiagnostics& diagnostics, const FrontendCTargetInfo& target,
	FrontendCTypeInfo& type_info) {
	(void)syntax_tree;
	for (FrontendCEnum& enumeration : type_info.enums) {
		if (!enumeration.body) continue;
		uni::Nnode* statement = const_cast<uni::Nnode*>(enumeration.declaration);
		uni::Nnode* body = const_cast<uni::Nnode*>(enumeration.body);
		if (!body || body->type != tok_block || !body->subf) {
			diagnostics.Report(DiagnosticStage::Parser,
				"enum requires at least one enumerator", statement);
			return false;
		}
		uni::Vector<uni::Nnode*> nodes;
		for (uni::Nnode* node = body->subf; node; node = node->next)
			AppendEnumeratorNodes(node, nodes);
		// C99 permits one comma after the final enumerator.
		if (nodes.Count()) {
			uni::Nnode* tail = nodes.begin()[nodes.Count() - 1];
			if (tail && tail->type == tok_symbol && tail->addr &&
				!StrCompare(tail->addr, ",")) nodes.Remove(nodes.Count() - 1);
		}
		if (!nodes.Count()) {
			diagnostics.Report(DiagnosticStage::Parser,
				"enum requires at least one enumerator", statement);
			return false;
		}
		stdsint next_value = 0;
		stdsint minimum = 0;
		stdsint maximum = 0;
		for (stduint index = 0; index < nodes.Count(); index++) {
			uni::Nnode* node = nodes.begin()[index];
			uni::Nnode* name_node = node;
			uni::Nnode* value_node = nullptr;
			if (node->type == tok_func && node->addr &&
				!StrCompare(node->addr, "OP@ASSIGN")) {
				name_node = node->subf;
				value_node = name_node ? name_node->next : nullptr;
				if (!name_node || !value_node || value_node->next) name_node = nullptr;
			}
			if (!name_node || name_node->type != tok_identy || !name_node->addr) {
				diagnostics.Report(DiagnosticStage::Parser,
					"invalid enum enumerator", node);
				return false;
			}
			if (EnumeratorExistsInScope(
				type_info, name_node->addr, enumeration.scope)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"redefinition of enumerator", name_node);
				return false;
			}
			stdsint value = next_value;
			if (value_node && !EvaluateEnumeratorValue(
				value_node, target, type_info, value)) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"enum value must be an integer constant", value_node);
				return false;
			}
			FrontendCEnumerator enumerator;
			enumerator.name = name_node->addr;
			enumerator.value = value;
			enumerator.declaration = name_node;
			enumeration.enumerators.Append(enumerator);
			if (!index || value < minimum) minimum = value;
			if (!index || value > maximum) maximum = value;
			if (index + 1 < nodes.Count() && value == SignedMaximum()) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"implicit enum value overflows the supported integer range", name_node);
				return false;
			}
			if (index + 1 < nodes.Count()) next_value = value + 1;
		}
		if (!SelectEnumCompatibleType(minimum, maximum, target,
			enumeration.integer_rank, enumeration.is_signed)) {
			diagnostics.Report(DiagnosticStage::Semantic,
				"enum values exceed the supported integer range", statement);
			return false;
		}
		enumeration.complete = true;
		FrontendCType compatible_type;
		compatible_type.integer_rank = enumeration.integer_rank;
		compatible_type.is_signed = enumeration.is_signed;
		for (FrontendCEnumerator& enumerator : enumeration.enumerators)
			enumerator.type = compatible_type;
	}
	for (FrontendCTypedNode& typed_node : type_info.nodes)
		ApplyEnumCompatibleType(typed_node.type, type_info);
	for (FrontendCRecord& record : type_info.records)
		for (FrontendCRecordMember& member : record.members)
			ApplyEnumCompatibleType(member.type, type_info);
	return true;
}

bool frontend_c_detail::Validate(uni::Nchain& syntax_tree,
	const FrontendCDiagnostics& diagnostics) {
	for (uni::Nnode* node = syntax_tree.Root(); node;) {
		if (node->type == ToToken(FrontendCToken::GlobalDeclaration) ||
			node->type == ToToken(FrontendCToken::RecordDeclaration) ||
			node->type == ToToken(FrontendCToken::RecordDefinition) ||
			node->type == ToToken(FrontendCToken::EnumDeclaration) ||
			node->type == ToToken(FrontendCToken::EnumDefinition)) {
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


