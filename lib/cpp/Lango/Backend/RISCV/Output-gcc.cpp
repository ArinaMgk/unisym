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
