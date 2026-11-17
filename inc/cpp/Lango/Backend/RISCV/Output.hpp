// ASCII C++11 TAB4 LF
// Docutitle: RISC-V Assembly Output Interface
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

#ifndef UNISYM_LANGO_BACKEND_RISCV_OUTPUT_HPP
#define UNISYM_LANGO_BACKEND_RISCV_OUTPUT_HPP

#include "../../Backend.hpp"

namespace uni::lango {

// Textual assembly dialect selected independently from the RISC-V target.
enum class RISCVAssemblySyntax {
	GCC, // GNU assembler syntax accepted by GCC toolchains.
	AASM // Unisym assembler syntax.
};

// Dialect-specific directives used by the common RISC-V lowering path.
class RISCVAssemblyOutputInterface {
public:
	virtual ~RISCVAssemblyOutputInterface() = default;

	// Emit an address using either a locally bound PC-relative relocation or GOT.
	virtual bool EmitAddress(const char* symbol, bool through_got,
		bool external, bool weak, clover::SymbolVisibility visibility,
		unsigned bits, uni::OstreamTrait& output) = 0;

	// Emit a direct call with the selected local or PLT relocation.
	virtual bool EmitDirectCall(const char* symbol, bool through_plt,
		uni::OstreamTrait& output) = 0;

	// Emit the ELF-facing attributes for a module symbol.
	virtual bool EmitSymbol(const char* symbol, bool function, bool weak,
		clover::SymbolVisibility visibility, bool external, bool declaration,
		uni::OstreamTrait& output) = 0;

	// Emit source-file declarations before assembly carrying line information.
	virtual bool EmitSourceFiles(const clover::Module& module,
		uni::OstreamTrait& output) = 0;

	// Select the source position for the next emitted machine instruction.
	virtual bool EmitLocation(uint32 file, uint32 line,
		uni::OstreamTrait& output) = 0;
};

}

#endif
