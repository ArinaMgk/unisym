// ASCII C++11 TAB4 LF
// Docutitle: Clover IR Optimizer
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

#include "../../inc/c/file.h"
#include "../../inc/c/consio.h"
#include "../../inc/cpp/Lango/CloverIR.hpp"
#include "../../inc/cpp/Lango/CloverPass.hpp"

static void PrintHelp() {
	uni::Console.OutFormat("Usage: clover-opt input.ir [options]\n");
	uni::Console.OutFormat("Options:\n");
	uni::Console.OutFormat("  -o output.ir              Write Clover IR to output.ir\n");
	uni::Console.OutFormat("  --passes=forward-jumps    Remove forwarding jump blocks\n");
	uni::Console.OutFormat("  -h, --help                Print this help\n");
}

int main(int argc, char** argv) {
	using namespace uni;

	rostr input_path = nullptr;
	rostr output_path = nullptr;
	rostr passes = nullptr;
	for (int index = 1; index < argc; index++) {
		if (!StrCompare(argv[index], "-h") || !StrCompare(argv[index], "--help")) {
			PrintHelp();
			return 0;
		}
		else if (!StrCompare(argv[index], "-o")) {
			if (++index >= argc) {
				plogerro("clover-opt: expected filename after -o");
				return 1;
			}
			output_path = argv[index];
		}
		else if (!StrCompareN(argv[index], "--passes=", 9)) {
			if (passes) {
				plogerro("clover-opt: --passes may only be specified once");
				return 1;
			}
			passes = argv[index] + 9;
			if (StrCompare(passes, "forward-jumps")) {
				plogerro("clover-opt: unsupported pass list %s; supported value: forward-jumps", passes);
				return 1;
			}
		}
		else if (argv[index][0] == '-') {
			plogerro("clover-opt: unknown option %s", argv[index]);
			return 1;
		}
		else if (!input_path) input_path = argv[index];
		else {
			plogerro("clover-opt: only one input file is supported");
			return 1;
		}
	}

	if (!input_path) {
		PrintHelp();
		return 0;
	}

	HostFile input(input_path, FileOpenType::Read);
	if (!input) {
		plogerro("clover-opt: cannot open input file %s", input_path);
		return 1;
	}

	clover::Module module;
	if (!clover::Parse(input, module)) return 1;
	if (passes && !clover::ForwardJumpBlocks(module).success) {
		plogerro("clover-opt: forward-jumps failed");
		return 1;
	}

	HostFile* output_file = nullptr;
	OstreamTrait* output = &Console;
	if (output_path) {
		output_file = new HostFile(output_path, FileOpenType::Write);
		if (!*output_file) {
			plogerro("clover-opt: cannot open output file %s", output_path);
			delete output_file;
			return 1;
		}
		output = output_file;
	}

	bool success = clover::Print(module, *output);
	if (output_file) delete output_file;
	return success ? 0 : 1;
}
