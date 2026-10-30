// ASCII C++11 TAB4 LF
// Docutitle: Lango C Frontend
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

#include "../../../../inc/cpp/Lango/Frontend/Frontend-c.hpp"
#include "../../../../inc/cpp/Lango/Frontend/C/Frontend-c-internal.hpp"

namespace uni::lango {

	namespace {
		bool IsHexDigits(const char* text) {
			if (!text || !*text) return false;
			for (; *text; text++)
				if (!((*text >= '0' && *text <= '9') ||
					(*text >= 'a' && *text <= 'f') ||
					(*text >= 'A' && *text <= 'F'))) return false;
			return true;
		}

		// The generic lexer separates the alphabetic tail of a hexadecimal token.
		void JoinCHexadecimalTokens(Dchain& tokens) {
			for (Dnode* node = tokens.Root(); node && node->next; node = node->next) {
				Dnode* tail = node->next;
				if (node->type != tok_number || tail->type != tok_identy ||
					!node->addr || !tail->addr || StrLength(node->addr) < 2 ||
					node->addr[0] != '0' ||
					(node->addr[1] != 'x' && node->addr[1] != 'X') ||
					!IsHexDigits(tail->addr)) continue;
				TnodeField* head_field = node->GetTnodeField();
				TnodeField* tail_field = tail->GetTnodeField();
				if (head_field->row != tail_field->row ||
					tail_field->col != head_field->col + StrLength(node->addr)) continue;
				char* joined = StrHeapAppend(node->addr, tail->addr);
				srs(node->addr, joined);
				tokens.Remove(tail);
			}
		}

		bool ValidateOptions(const FrontendOptions& options,
			const FrontendCDiagnostics& diagnostics) {
			if (options.language_standard &&
				StrCompare(options.language_standard, "c99")) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"unsupported C standard; supported standard: c99");
				return false;
			}
			FrontendCTargetInfo target = FrontendCTargetInfo::FromTarget(options.target);
			if (target.int_bits != 32) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"unsupported target int width; supported width: 32");
				return false;
			}
			return true;
		}

		bool AnalyzeC(NestedParseUnit& parser, const FrontendCDiagnostics& diagnostics,
			const FrontendCTargetInfo& target, FrontendCTypeInfo& type_info,
			FrontendCDebugInfo* debug_info) {
			Nchain& syntax_tree = *parser.GetNetwork();
			if (!parser.ParseParen(syntax_tree.Root())) {
				diagnostics.Report(DiagnosticStage::Parser,
					"unmatched or invalid delimiter", syntax_tree.Root());
				return false;
			}
			parser.ParseStatements_CPL();
			if (!frontend_c_detail::Prepare(syntax_tree, diagnostics, target,
				type_info, debug_info)) return false;
			if (!parser.ParseOperator(syntax_tree.Root())) {
				diagnostics.Report(DiagnosticStage::Parser,
					"invalid operator expression", syntax_tree.Root());
				return false;
			}
			return frontend_c_detail::Validate(syntax_tree, diagnostics);
		}

		bool ParseC(IstreamTrait& input, NodeChain& operators,
			NestedParseUnit*& syntax, const FrontendCDiagnostics& diagnostics,
			const FrontendCTargetInfo& target, FrontendCTypeInfo& type_info,
			FrontendCDebugInfo* debug_info) {
			LinearParser lexer(input);
			Dchain tokens;
			lexer.GHT = false;
			lexer.handler_comment = LineParse_Comment_C_Cpp;
			lexer.method_omit_comment = true;
			lexer.Parse(tokens);
			JoinCHexadecimalTokens(tokens);

			frontend_c_detail::ListOperators(operators);
			syntax = new NestedParseUnit(tokens, &operators, sizeof(mag_node_t));
			if (AnalyzeC(*syntax, diagnostics, target, type_info, debug_info)) return true;
			delete syntax;
			syntax = nullptr;
			return false;
		}

		void PrintNode(OstreamTrait& output, Nnode* node, stduint nest = 0) {
			for (; node; node = node->next) {
				for0(index, nest) output.OutFormat(index + 1 == _LIMIT ? "->" : "--");
				output.OutFormat("|%s|\n", node->addr ? node->addr : "(null)");
				if (node->subf) PrintNode(output, node->subf, nest + 1);
			}
		}

	}

	bool FrontendC::Generate(IstreamTrait& input, clover::Module& module,
		const FrontendOptions& options) {
		NodeChain operators(NodeHeapFreeSimple);
		NestedParseUnit* syntax = nullptr;
		FrontendCDiagnostics diagnostics;
		diagnostics.output = options.diagnostics ? options.diagnostics :
			&DefaultDiagnosticInterface();
		diagnostics.source_name = options.source_name ? options.source_name : "<input>";
		if (!ValidateOptions(options, diagnostics)) return false;
		FrontendCTargetInfo target = FrontendCTargetInfo::FromTarget(options.target);
		FrontendCTypeInfo type_info;
		FrontendCDebugInfo debug_info;
		FrontendCDebugInfo* requested_debug_info = options.debug_source ? &debug_info : nullptr;
		if (!ParseC(input, operators, syntax, diagnostics, target,
			type_info, requested_debug_info)) return false;
		bool success = frontend_c_detail::Lower(
			*syntax->GetNetwork(), module, diagnostics, target,
			type_info, requested_debug_info);
		delete syntax;
		return success;
	}

	bool FrontendC::PrintAST(IstreamTrait& input, OstreamTrait& output,
		const FrontendOptions& options) {
		NodeChain operators(NodeHeapFreeSimple);
		NestedParseUnit* syntax = nullptr;
		FrontendCDiagnostics diagnostics;
		diagnostics.output = options.diagnostics ? options.diagnostics :
			&DefaultDiagnosticInterface();
		diagnostics.source_name = options.source_name ? options.source_name : "<input>";
		if (!ValidateOptions(options, diagnostics)) return false;
		FrontendCTargetInfo target = FrontendCTargetInfo::FromTarget(options.target);
		FrontendCTypeInfo type_info;
		if (!ParseC(input, operators, syntax, diagnostics, target,
			type_info, nullptr)) return false;
		PrintNode(output, syntax->GetNetwork()->Root());
		delete syntax;
		return true;
	}

}

void uni::lango::FrontendCDiagnostics::Report(DiagnosticStage stage,
	const char* message, const uni::Nnode* node) const {
	Diagnostic diagnostic;
	diagnostic.stage = stage;
	diagnostic.message = message;
	diagnostic.location.file = source_name ? source_name : "<input>";
	if (node) {
		const uni::TnodeField* field =
			const_cast<uni::Nnode*>(node)->GetTnodeField();
		diagnostic.location.row = field->row;
		diagnostic.location.column = field->col;
	}
	(output ? output : &DefaultDiagnosticInterface())->Report(diagnostic);
}
