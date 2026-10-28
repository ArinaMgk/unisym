// ASCII C++11 TAB4 LF
// Docutitle: Clover IR to RISC-V Compiler
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
#include "../../inc/cpp/Lango/Backend/Backend-riscv.hpp"

// Print command syntax and the target features implemented by this backend.
static void PrintHelp() {
	uni::Console.OutFormat("Usage: clover-riscv input.ir [options]\n");
	uni::Console.OutFormat("Options:\n");
	uni::Console.OutFormat("  -o output.s   Write assembly to output.s\n");
	uni::Console.OutFormat("  -mbits=64     Select register width\n");
	uni::Console.OutFormat("  -minst=i      Select the base integer instruction set\n");
	uni::Console.OutFormat("  -minst=im     Enable integer multiply and divide\n");
	uni::Console.OutFormat("  -h            Print this help\n");
	uni::Console.OutFormat("Supported bits: 64\n");
	uni::Console.OutFormat("Supported inst: i, m\n");
	uni::Console.OutFormat("Default target: -mbits=64 -minst=im\n");
}

int main(int argc, char** argv) {
	using namespace uni;

	rostr input_path = nullptr;
	rostr output_path = nullptr;
	uni::lango::RISCVTargetInfo target;
	for (int index = 1; index < argc; index++) {
		if (!StrCompare(argv[index], "-h") || !StrCompare(argv[index], "--help")) {
			PrintHelp();
			return 0;
		}
		else if (!StrCompare(argv[index], "-o")) {
			if (++index >= argc) {
				plogerro("clover-riscv: expected filename after -o");
				return 1;
			}
			output_path = argv[index];
		}
		else if (!StrCompareN(argv[index], "-mbits=", 7)) {
			rostr value = argv[index] + 7;
			if (StrCompare(value, "64")) {
				plogerro("clover-riscv: unsupported -mbits=%s; supported bits: 64", value);
				return 1;
			}
			target.bits = 64;
		}
		else if (!StrCompareN(argv[index], "-minst=", 7)) {
			rostr value = argv[index] + 7;
			if (!StrCompare(value, "i")) target.extension_m = false;
			else if (!StrCompare(value, "im")) target.extension_m = true;
			else {
				plogerro("clover-riscv: unsupported -minst=%s; supported values: i, im", value);
				return 1;
			}
			target.extension_i = true;
		}
		else if (argv[index][0] == '-') {
			plogerro("clover-riscv: unknown option %s", argv[index]);
			return 1;
		}
		else if (!input_path) input_path = argv[index];
		else {
			plogerro("clover-riscv: only one input file is supported");
			return 1;
		}
	}

	if (!input_path) {
		PrintHelp();
		return 0;
	}

	HostFile input(input_path, FileOpenType::Read);
	if (!input) {
		plogerro("clover-riscv: cannot open input file %s", input_path);
		return 1;
	}

	HostFile* output_file = nullptr;
	OstreamTrait* output = &Console;
	if (output_path) {
		output_file = new HostFile(output_path, FileOpenType::Write);
		if (!*output_file) {
			plogerro("clover-riscv: cannot open output file %s", output_path);
			delete output_file;
			return 1;
		}
		output = output_file;
	}

	clover::Module module;
	uni::lango::BackendRISCV backend(target);
	bool success = clover::Parse(input, module) && backend.Generate(module, *output);
	if (output_file) delete output_file;
	return success ? 0 : 1;
}
