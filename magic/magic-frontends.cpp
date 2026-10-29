// ASCII C++11 TAB4 LF
// Docutitle: Magice Frontend Dispatcher
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

#include "./magice.hpp"

bool FrontendAnalyze(MagicFrontend frontend, uni::NestedParseUnit& parser) {
	switch (frontend) {
	case MagicFrontend::C: {
		uni::Nchain& syntax_tree = *parser.GetNetwork();
		if (!parser.ParseParen(syntax_tree.Root())) return false;
		parser.ParseStatements_CPL();
		if (!FrontendCPrepare(syntax_tree)) return false;
		if (!parser.ParseOperator(syntax_tree.Root())) return false;
		return FrontendCValidate(syntax_tree);
	}
	}
	plogerro("Magice: unsupported frontend");
	return false;
}
