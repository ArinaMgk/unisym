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

		bool AnalyzeC(NestedParseUnit& parser, FrontendCDebugInfo* debug_info) {
			Nchain& syntax_tree = *parser.GetNetwork();
			if (!parser.ParseParen(syntax_tree.Root())) return false;
			parser.ParseStatements_CPL();
			if (!frontend_c_detail::Prepare(syntax_tree, debug_info)) return false;
			if (!parser.ParseOperator(syntax_tree.Root())) return false;
			return frontend_c_detail::Validate(syntax_tree);
		}

		bool ParseC(IstreamTrait& input, NodeChain& operators,
			NestedParseUnit*& syntax, FrontendCDebugInfo* debug_info) {
			LinearParser lexer(input);
			Dchain tokens;
			lexer.GHT = false;
			lexer.handler_comment = LineParse_Comment_C_Cpp;
			lexer.method_omit_comment = true;
			lexer.Parse(tokens);

			frontend_c_detail::ListOperators(operators);
			syntax = new NestedParseUnit(tokens, &operators, sizeof(mag_node_t));
			if (AnalyzeC(*syntax, debug_info)) return true;
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
		FrontendCDebugInfo debug_info;
		FrontendCDebugInfo* requested_debug_info = options.debug_source ? &debug_info : nullptr;
		if (!ParseC(input, operators, syntax, requested_debug_info)) return false;
		bool success = frontend_c_detail::Lower(
			*syntax->GetNetwork(), module, requested_debug_info);
		delete syntax;
		return success;
	}

	bool FrontendC::PrintAST(IstreamTrait& input, OstreamTrait& output) {
		NodeChain operators(NodeHeapFreeSimple);
		NestedParseUnit* syntax = nullptr;
		if (!ParseC(input, operators, syntax, nullptr)) return false;
		PrintNode(output, syntax->GetNetwork()->Root());
		delete syntax;
		return true;
	}

}
