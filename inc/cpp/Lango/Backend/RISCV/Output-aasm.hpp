// ASCII C++11 TAB4 LF
// Docutitle: RISC-V AASM Assembly Output
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

#ifndef UNISYM_LANGO_BACKEND_RISCV_OUTPUT_AASM_HPP
#define UNISYM_LANGO_BACKEND_RISCV_OUTPUT_AASM_HPP

#include "./Output.hpp"

namespace uni::lango {

// Translate the common RISC-V lowering stream to Unisym assembler syntax.
class RISCVAssemblyOutputAASM : public RISCVAssemblyOutputInterface,
	public uni::OstreamTrait {
private:
	uni::OstreamTrait& destination;
	uni::String line;
	const clover::Module* source_module = nullptr;
	unsigned target_bits;
	uni::String target_instructions;
	uni::String target_abi;
	bool bss_section = false;
	unsigned address_label = 0;

	// Convert and write one complete assembly line.
	bool EmitLine();

public:
	RISCVAssemblyOutputAASM(unsigned bits, const char* instructions,
		const char* abi, uni::OstreamTrait& output);

	// Emit target selection directives before sections and instructions.
	bool Begin();

	// Receive the common lowering stream and translate complete lines.
	virtual int out(const char* text, stduint length) override;

	// Write a final unterminated line, if one exists.
	bool Flush();
	virtual bool EmitAddress(const char* symbol, bool through_got,
		bool external, bool weak, clover::SymbolVisibility visibility,
		unsigned bits, uni::OstreamTrait& output) override;
	virtual bool EmitDirectCall(const char* symbol, bool through_plt,
		uni::OstreamTrait& output) override;
	virtual bool EmitSymbol(const char* symbol, bool function, bool weak,
		clover::SymbolVisibility visibility, bool external, bool declaration,
		uni::OstreamTrait& output) override;

	virtual bool EmitSourceFiles(const clover::Module& module,
		uni::OstreamTrait& output) override;
	virtual bool EmitLocation(uint32 file, uint32 line,
		uni::OstreamTrait& output) override;
};

}

#endif
