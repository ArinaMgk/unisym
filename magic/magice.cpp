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

#include "../inc/cpp/parse.hpp"
#include "../inc/cpp/vector"
#include "./include/clover-ir.hpp"



uni::OstreamTrait* dst;// to be a HostFile or ConsoleLine
class MagInn : public uni::IstreamTrait {
public:
	char* p;
	MagInn(char* p = 0) : p(p) {}
	virtual int inn() {
		return *p ? *p++ : -1;
	}

};

// Print one AST node using the existing tree indentation convention.
static void NnodePrint(uni::Nnode& nnod, stduint nest) {
	for0(i, nest) dst->OutFormat(i + 1 == _LIMIT ? "->" : "--");
	dst->OutFormat("|%s|\n", nnod.addr ? nnod.addr : "(null)");
}

// Return true when a path uses the C source extension.
static bool IsCSourcePath(rostr path) {
	stduint length = StrLength(path);
	return length >= 2 && path[length - 2] == '.' &&
		(path[length - 1] == 'c' || path[length - 1] == 'C');
}

int magic(int argc, char** argv) {
	using namespace uni;
	MagInn minn;
	LinearParser parser(minn);
	Dchain dc;
	NodeChain operators(NodeHeapFreeSimple);
	HostFile* output_file = nullptr;
	bool request_ir = false;
	bool request_ast = false;
	bool has_c_source = false;
	rostr output_path = nullptr;
	Vector<rostr> inputs;
	for (int i = 1; i < argc; i++) {
		if (!StrCompare(argv[i], "--emit-ir") || !StrCompare(argv[i], "-emit-ir")) {
			request_ir = true;
		}
		else if (!StrCompare(argv[i], "--emit-ast") || !StrCompare(argv[i], "-emit-ast")) {
			request_ast = true;
		}
		else if (!StrCompare(argv[i], "-o")) {
			if (++i >= argc) {
				plogerro("expected filename after -o");
				return 1;
			}
			output_path = argv[i];
		}
		else {
			inputs.Append(argv[i]);
			if (IsCSourcePath(argv[i])) has_c_source = true;
		}
	}

	dst = &Console;
	if (request_ir && request_ast) {
		plogerro("-emit-ir and -emit-ast cannot be used together");
		return 1;
	}

	if (inputs.isEmpty()) {
		ploginfo("Usage: mgc [-emit-ir|-emit-ast] input... [-o output]");
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
		dst = output_file;
	}

	parser.GHT = false;
	for (rostr input : inputs) {
		HostFile input_file(input, FileOpenType::Read);
		if (input_file) {
			String input_source;
			input_file >> input_source;
			minn.p = (char*)input_source.reference();
			parser.Parse(dc);
		}
		else {
			minn.p = (char*)input;
			parser.Parse(dc);
		}
	}

	{
		Operators::List(operators);

		NestedParseUnit fileScope(dc, &operators, sizeof(mag_node_t));
		NestedParseUnit& npu = fileScope;

		if (!npu.Parse()) {
			if (output_file) delete output_file;
			return 1;
		}

		npu.ParseStatements_CPL();
		if (emit_ast) {
			npu.GetNetwork()->Traversal(NnodePrint);
		}
		else {
			clover::Module module;
			if (!clover::Generate(*fileScope.GetNetwork(), module) || !clover::Print(module, *dst)) {
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
