// ASCII C++11 TAB4 LF
// Docutitle: Lango SystemVerilog Frontend
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

#ifndef UNISYM_LANGO_FRONTEND_SYSTEMVERILOG_HPP
#define UNISYM_LANGO_FRONTEND_SYSTEMVERILOG_HPP

#include "../Frontend.hpp"

namespace uni::lango {

class FrontendSystemVerilog : public FrontendInterface {
public:
	virtual bool Generate(uni::IstreamTrait& input, clover::Module& module,
		const FrontendOptions& options = FrontendOptions()) override;

	bool PrintAST(uni::IstreamTrait& input, uni::OstreamTrait& output,
		const FrontendOptions& options = FrontendOptions());
};

}

#endif
