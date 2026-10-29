// ASCII C++11 TAB4 LF
// Docutitle: Lango Frontend Interface
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

#ifndef UNISYM_LANGO_FRONTEND_HPP
#define UNISYM_LANGO_FRONTEND_HPP

#include "./CloverIR.hpp"

namespace uni::lango {

// Options shared by source-language frontends.
struct FrontendOptions {
	bool debug_source = false; // Retain source statements in generated Clover IR.
};

// Common interface implemented by source-language Clover IR frontends.
class FrontendInterface {
public:
	virtual ~FrontendInterface() = default;

	// Parse one translation unit and generate its in-memory Clover module.
	virtual bool Generate(uni::IstreamTrait& input, clover::Module& module,
		const FrontendOptions& options = FrontendOptions()) = 0;
};

}

#endif
