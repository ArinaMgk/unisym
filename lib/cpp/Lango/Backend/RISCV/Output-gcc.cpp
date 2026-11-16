// ASCII C++11 TAB4 LF
// Docutitle: RISC-V GCC Assembly Output
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

#include "../../../../../inc/cpp/Lango/Backend/RISCV/Output-gcc.hpp"

namespace uni::lango {

namespace {

	void EmitQuotedPath(const uni::String& path, uni::OstreamTrait& output) {
		output.OutChar('"');
		for (const char* current = path.reference(); current && *current; current++) {
			if (*current == '"' || *current == '\\') output.OutChar('\\');
			output.OutChar(*current);
		}
		output.OutChar('"');
	}

}

bool RISCVAssemblyOutputGCC::EmitAddress(const char* symbol, bool through_got,
	bool, bool, clover::SymbolVisibility, unsigned, uni::OstreamTrait& output) {
	output.OutFormat("\t%s t0, %s\n", through_got ? "lga" : "lla", symbol);
	return true;
}

bool RISCVAssemblyOutputGCC::EmitSymbol(const char* symbol, bool function,
	bool weak, clover::SymbolVisibility visibility, bool external, bool,
	uni::OstreamTrait& output) {
	if (external)
		output.OutFormat(".%s %s\n", weak ? "weak" : "globl", symbol);
	if (visibility == clover::SymbolVisibility::Hidden)
		output.OutFormat(".hidden %s\n", symbol);
	else if (visibility == clover::SymbolVisibility::Protected)
		output.OutFormat(".protected %s\n", symbol);
	output.OutFormat(".type %s, @%s\n", symbol,
		function ? "function" : "object");
	return true;
}

bool RISCVAssemblyOutputGCC::EmitSourceFiles(const clover::Module& module,
	uni::OstreamTrait& output) {
	for (const clover::SourceFile& source : module.source_files) {
		if (!source.id || !source.path.getByteCount()) return false;
		output.OutFormat(".file %[u] ", source.id);
		EmitQuotedPath(source.path, output);
		output.OutChar('\n');
	}
	return true;
}

bool RISCVAssemblyOutputGCC::EmitLocation(uint32 file, uint32 line,
	uni::OstreamTrait& output) {
	if (!file || !line) return false;
	output.OutFormat("\t.loc %[u] %[u] 0\n", file, line);
	return true;
}

}
