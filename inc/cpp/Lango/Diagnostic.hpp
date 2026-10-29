// ASCII C++11 TAB4 LF
// Docutitle: Lango Diagnostic Interface
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

#ifndef UNISYM_LANGO_DIAGNOSTIC_HPP
#define UNISYM_LANGO_DIAGNOSTIC_HPP

#include "../string"

namespace uni::lango {

// Importance assigned to one compiler diagnostic.
enum class DiagnosticLevel {
	Note,    // Additional information related to another diagnostic.
	Warning, // Suspicious input which does not stop compilation.
	Error    // Invalid input or an unsupported required operation.
};

// Compiler stage which produced one diagnostic.
enum class DiagnosticStage {
	Lexer,    // Source character and token recognition.
	Parser,   // Source grammar and syntax-tree construction.
	Semantic, // Declaration, scope, and type analysis.
	Lowering, // Translation from a frontend tree to Clover IR.
	Backend   // Translation from Clover IR to a target representation.
};

// Beginning of the source construct associated with a diagnostic.
struct SourceLocation {
	uni::String file;
	stduint row = 0;
	stduint column = 0;
};

// One compiler diagnostic independent of its final presentation.
struct Diagnostic {
	DiagnosticLevel level = DiagnosticLevel::Error;
	DiagnosticStage stage = DiagnosticStage::Parser;
	SourceLocation location;
	uni::String message;
};

// Destination supplied by a driver or embedding application for diagnostics.
class DiagnosticInterface {
public:
	virtual ~DiagnosticInterface() = default;
	virtual void Report(const Diagnostic& diagnostic) = 0;
};

// Return the process-wide logger used when a caller provides no destination.
DiagnosticInterface& DefaultDiagnosticInterface();

}

#endif
