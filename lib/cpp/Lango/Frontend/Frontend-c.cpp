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
#include "../../../../inc/cpp/Lango/Frontend/C/Frontend-c-preprocessor.hpp"

namespace uni::lango {

	namespace {
		void NormalizeCEllipsis(Dchain& tokens) {
			for (Dnode* node = tokens.Root(); node; node = node->next) {
				if (node->addr && !StrCompare(node->addr, "...")) {
					node->type = tok_symbol;
					continue;
				}
				Dnode* second = node->next;
				Dnode* third = second ? second->next : nullptr;
				if (!node->addr || !second || !second->addr || !third || !third->addr ||
					StrCompare(node->addr, ".") || StrCompare(second->addr, ".") ||
					StrCompare(third->addr, ".")) continue;
				TnodeField* first_field = node->GetTnodeField();
				TnodeField* second_field = second->GetTnodeField();
				TnodeField* third_field = third->GetTnodeField();
				if (first_field->row != second_field->row ||
					first_field->row != third_field->row ||
					second_field->col != first_field->col + 1 ||
					third_field->col != second_field->col + 1) continue;
				srs(node->addr, StrHeap("..."));
				node->type = tok_symbol;
				tokens.Remove(second);
				tokens.Remove(third);
			}
		}

		void NormalizeCReturnKeyword(Dchain& tokens) {
			for (Dnode* node = tokens.Root(); node; node = node->next)
				if (node->type == tok_identy && node->addr &&
					!StrCompare(node->addr, "return"))
					node->type = ToToken(FrontendCToken::Return);
		}

		bool ResolveOptions(const FrontendOptions& options,
			const FrontendCDiagnostics& diagnostics, FrontendCTargetInfo& target) {
			if (options.language_standard &&
				StrCompare(options.language_standard, "c99")) {
				diagnostics.Report(DiagnosticStage::Semantic,
					"unsupported C standard; supported standard: c99");
				return false;
			}
			target = FrontendCTargetInfo::FromTarget(options.target);
			bool has_data_model = false;
			for (const FrontendLanguageOption& option : options.language_options) {
				if (StrCompare(option.name.reference(), "data-model")) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"unsupported C frontend option");
					return false;
				}
				if (has_data_model) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"duplicate C frontend option: data-model");
					return false;
				}
				if (!option.has_value || !option.value.getByteCount()) {
					diagnostics.Report(DiagnosticStage::Semantic,
						"C frontend option data-model requires a value");
					return false;
				}
				CDataModel data_model;
				if (!StrCompare(option.value.reference(), "ilp32"))
					data_model = CDataModel::ILP32;
				else if (!StrCompare(option.value.reference(), "lp64"))
					data_model = CDataModel::LP64;
				else if (!StrCompare(option.value.reference(), "llp64"))
					data_model = CDataModel::LLP64;
				else {
					diagnostics.Report(DiagnosticStage::Semantic,
						"unsupported C data model; supported values: ilp32, lp64, llp64");
					return false;
				}
				target = FrontendCTargetInfo::FromDataModel(data_model);
				has_data_model = true;
			}
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
			// C must retain single-item parentheses until type names are distinguished
			// from grouped expressions such as (identifier).
			if (!parser.ParseParen(syntax_tree.Root(), false)) {
				diagnostics.Report(DiagnosticStage::Parser,
					"unmatched or invalid delimiter", syntax_tree.Root());
				return false;
			}
			if (!frontend_c_detail::PreserveCompoundLiteralSyntax(syntax_tree))
				return false;
			parser.ParseStatements_CPL();
			if (!frontend_c_detail::Prepare(syntax_tree, diagnostics, target,
				type_info, debug_info)) return false;
			if (!frontend_c_detail::PrepareCompoundLiterals(
				syntax_tree, diagnostics, target, type_info)) return false;
			if (!parser.ParseOperator(syntax_tree.Root())) {
				diagnostics.Report(DiagnosticStage::Parser,
					"invalid postfix operator expression", syntax_tree.Root());
				return false;
			}
			if (!frontend_c_detail::ParseCastOperators(
				syntax_tree, diagnostics, target, type_info)) return false;
			frontend_c_detail::ListPrefixOperators(
				*parser.TokenOperatorGroupChain);
			if (!parser.ParseOperator(syntax_tree.Root())) {
				diagnostics.Report(DiagnosticStage::Parser,
					"invalid prefix or cast expression", syntax_tree.Root());
				return false;
			}
			if (!frontend_c_detail::ParseCallExpressions(syntax_tree)) return false;
			frontend_c_detail::ListBinaryOperators(
				*parser.TokenOperatorGroupChain);
			if (!parser.ParseOperator(syntax_tree.Root())) {
				diagnostics.Report(DiagnosticStage::Parser,
					"invalid binary operator expression", syntax_tree.Root());
				return false;
			}
			if (!frontend_c_detail::ParseConditionalOperators(
				syntax_tree, diagnostics)) return false;
			frontend_c_detail::ListTrailingOperators(
				*parser.TokenOperatorGroupChain);
			if (!parser.ParseOperator(syntax_tree.Root())) {
				diagnostics.Report(DiagnosticStage::Parser,
					"invalid trailing operator expression", syntax_tree.Root());
				return false;
			}
			if (!frontend_c_detail::FinalizeEnums(
				syntax_tree, diagnostics, target, type_info)) return false;
			if (!frontend_c_detail::FinalizeArrayBounds(
				diagnostics, target, type_info)) return false;
			if (!frontend_c_detail::AnalyzeControlFlow(
				syntax_tree, diagnostics, type_info)) return false;
			return frontend_c_detail::Validate(syntax_tree, diagnostics);
		}

		bool ParseC(IstreamTrait& input, NodeChain& operators,
			NestedParseUnit*& syntax, const FrontendCDiagnostics& diagnostics,
			const FrontendCTargetInfo& target, FrontendCTypeInfo& type_info,
			FrontendCDebugInfo* debug_info,
			const uni::Vector<uni::String>& include_paths) {
			Dchain tokens;
			frontend_c_preprocessor::Preprocessor preprocessor(
				diagnostics, target, include_paths);
			if (!preprocessor.Process(input, tokens)) return false;
			LinearParser::StringConcatenation(tokens);
			NormalizeCEllipsis(tokens);
			NormalizeCReturnKeyword(tokens);

			frontend_c_detail::ListPostfixOperators(operators);
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
		FrontendCTargetInfo target;
		if (!ResolveOptions(options, diagnostics, target)) return false;
		FrontendCTypeInfo type_info;
		FrontendCDebugInfo debug_info;
		FrontendCDebugInfo* requested_debug_info = options.debug_source ? &debug_info : nullptr;
		if (!ParseC(input, operators, syntax, diagnostics, target,
			type_info, requested_debug_info, options.include_paths)) return false;
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
		FrontendCTargetInfo target;
		if (!ResolveOptions(options, diagnostics, target)) return false;
		FrontendCTypeInfo type_info;
		if (!ParseC(input, operators, syntax, diagnostics, target,
			type_info, nullptr, options.include_paths)) return false;
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

void uni::lango::FrontendCDiagnostics::Report(DiagnosticStage stage,
	const char* message, stduint row, stduint column) const {
	Diagnostic diagnostic;
	diagnostic.stage = stage;
	diagnostic.message = message;
	diagnostic.location.file = source_name ? source_name : "<input>";
	diagnostic.location.row = row;
	diagnostic.location.column = column;
	(output ? output : &DefaultDiagnosticInterface())->Report(diagnostic);
}
