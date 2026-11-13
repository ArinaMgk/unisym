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

	// Emit source-file declarations before assembly carrying line information.
	virtual bool EmitSourceFiles(const clover::Module& module,
		uni::OstreamTrait& output) = 0;

	// Select the source position for the next emitted machine instruction.
	virtual bool EmitLocation(uint32 file, uint32 line,
		uni::OstreamTrait& output) = 0;
};

}

#endif
