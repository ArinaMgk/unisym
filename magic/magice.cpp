// ASCII C++ TAB4 LF
// Docutitle: Magice Compiler
// Datecheck: 20240416 ~ .
// Developer: @dosconio, @ArinaMgk
// Attribute: [Allocate]
// Reference: None
// Dependens: None
// Descriptn: C expression source -> AST or Clover IR
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

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
#define _CRT_SECURE_NO_WARNINGS 1

#include "./magice.hpp"


#include "../inc/c/uctype.h"
#include "../inc/c/file.h"

#include "../inc/cpp/vector"


class MagicSourceInput : public uni::IstreamTrait {
public:
	const char* source;
	MagicSourceInput(const char* text) : source(text) {}
	virtual int inn() {
		return *source ? *source++ : -1;
	}
};

// Return true when a path uses the C source extension.
static bool IsCSourcePath(rostr path) {
	stduint length = StrLength(path);
	return length >= 2 && path[length - 2] == '.' &&
		(path[length - 1] == 'c' || path[length - 1] == 'C');
}

static void PrintMagicHelp() {
	uni::Console.OutFormat("Usage: mgc [options] input...\n");
	uni::Console.OutFormat("Options:\n");
	uni::Console.OutFormat("  -emit-ir, --emit-ir    Emit Clover IR\n");
	uni::Console.OutFormat("  -emit-ast, --emit-ast  Emit the parsed AST\n");
	uni::Console.OutFormat("  -g                      Add source comments to Clover IR\n");
	uni::Console.OutFormat("  -o output               Write output to a file\n");
	uni::Console.OutFormat("  -h, --help              Print this help\n");
}

int magic(int argc, char** argv) {
	using namespace uni;
	HostFile* output_file = nullptr;
	OstreamTrait* output = &Console;
	bool request_ir = false;
	bool request_ast = false;
	bool debug_symbols = false;
	bool options_ended = false;
	bool has_c_source = false;
	rostr output_path = nullptr;
	Vector<rostr> inputs;
	for (int i = 1; i < argc; i++) {
		if (!options_ended && !StrCompare(argv[i], "--")) {
			options_ended = true;
		}
		else if (!options_ended &&
			(!StrCompare(argv[i], "--emit-ir") || !StrCompare(argv[i], "-emit-ir"))) {
			request_ir = true;
		}
		else if (!options_ended &&
			(!StrCompare(argv[i], "--emit-ast") || !StrCompare(argv[i], "-emit-ast"))) {
			request_ast = true;
		}
		else if (!options_ended && !StrCompare(argv[i], "-g")) {
			debug_symbols = true;
		}
		else if (!options_ended &&
			(!StrCompare(argv[i], "-h") || !StrCompare(argv[i], "--help"))) {
			PrintMagicHelp();
			return 0;
		}
		else if (!options_ended && !StrCompare(argv[i], "-o")) {
			if (++i >= argc) {
				plogerro("expected filename after -o");
				return 1;
			}
			output_path = argv[i];
		}
		else if (!options_ended && argv[i][0] == '-' && argv[i][1]) {
			plogerro("unknown option %s", argv[i]);
			return 1;
		}
		else {
			inputs.Append(argv[i]);
			if (IsCSourcePath(argv[i])) has_c_source = true;
		}
	}

	if (request_ir && request_ast) {
		plogerro("-emit-ir and -emit-ast cannot be used together");
		return 1;
	}

	if (inputs.isEmpty()) {
		PrintMagicHelp();
		return 0;
	}
	bool emit_ast = request_ast || (!request_ir && has_c_source);

	if (output_path) {
		output_file = new HostFile(output_path, FileOpenType::Write);
		if (!*output_file) {
			plogerro("cannot open output file %s", output_path);
			delete output_file;
			return 1;
		}
		output = output_file;
	}

	String source;
	const char* source_name = inputs.Count() == 1 ? "<command-line>" : "<multiple-inputs>";
	for (rostr input : inputs) {
		HostFile input_file(input, FileOpenType::Read);
		if (input_file) {
			if (inputs.Count() == 1) source_name = input;
			String input_source;
			input_file >> input_source;
			source += input_source;
		}
		else {
			String input_source(input);
			source += input_source;
		}
		source << '\n';
	}

	{
		MagicSourceInput input(source.reference());
		lango::FrontendOptions frontend_options;
		frontend_options.debug_source = debug_symbols;
		frontend_options.source_name = source_name;
		if (emit_ast) {
			if (!FrontendPrintAST(MagicFrontend::C, input, *output, frontend_options)) {
				if (output_file) delete output_file;
				return 1;
			}
		}
		else {
			lango::FrontendInterface* frontend = FrontendCreate(MagicFrontend::C);
			if (!frontend) {
				if (output_file) delete output_file;
				return 1;
			}
			clover::Module module;
			bool success = frontend->Generate(input, module, frontend_options) &&
				clover::Print(module, *output);
			delete frontend;
			if (!success) {
				if (output_file) delete output_file;
				return 1;
			}
		}
	}

	if (output_file) delete output_file;
	return 0;
}

const char* mgc_top_file_path = nullptr;

int main(int argc, char** argv)
{
	using namespace uni;
	int stat = 0;

	bool is_mark = false;

	int file_arg_idx = 1;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (!StrCompare(argv[i], "-f") || !StrCompare(argv[i], "-o")) {
				i++;
			}
		} else {
			file_arg_idx = i;
			break;
		}
	}
	if (file_arg_idx < argc) {
		// judge "mgc XXX.mgc"
		mgc_top_file_path = argv[file_arg_idx];
		HostFile file(argv[file_arg_idx], FileOpenType::Read);
		if (file) {
			String buf(String::Charset::UTF8, 22);
			for (stduint i = 0; i < 21; i++) {
				int ch = file.inn();
				if (ch == EOF) break;
				else buf << ch;
			}
			if (!StrCompare(buf.reference(), "#UTF-8 MAgicRK(1) . ."))
				is_mark = true;
		}
	}
	if (!is_mark) stat = magic(argc, argv);
	else stat = mark(argc, argv);
	if (malc_count) plogerro("Memory Leak %[u]", malc_count);
	return stat;
}
