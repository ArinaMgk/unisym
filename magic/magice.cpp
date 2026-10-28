// ASCII C++ TAB4 LF
// Docutitle: Magice Compiler
// Datecheck: 20240416 ~ .
// Developer: @dosconio, @ArinaMgk
// Attribute: [Allocate]
// Reference: None
// Dependens: None
// Descriptn: .mgc -> {.mid / .mak} for {.exe / .html}
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
#include "../inc/c/compile/asmcode.h"
#include "./include/clover-ir.hpp"



uni::OstreamTrait* dst;// to be a HostFile or ConsoleLine
uni::IstreamTrait* src;

char** arg_v;

class MagInn : public uni::IstreamTrait {
public:
	char* p;
	MagInn(char* p = 0) : p(p) {}
	virtual int inn() {
		return *p ? *p++ : -1;
	}

};



enum Architecture_t platform = Architecture_RISCV64;
// AASM Architecture_x86     
// GAS  Architecture_RISCV64



void mag_erro(rostr srcfile, stduint lineno, uni::Tnode* dn, rostr fmt, ...) {
	using namespace uni;
	Letpara(plist, fmt);
	printlogx(_LOG_ERROR, fmt, plist);
	//
	Console.OutFormat("%s\n", srcfile);
	if (false && dn->col) {
		for0(i, dn->col - 1) Console.OutFormat(" ");
		Console.OutFormat("^\n");
	}
}





static void push(void) {
	dst->OutFormat("\taddi sp, sp, -8 # push\n");
	dst->OutFormat("\tsd a0, 0(sp)\n");
	// Depth++;
}
static void pop(rostr reg) {
	dst->OutFormat("\tld %s, 0(sp) # pop\n", reg);
	dst->OutFormat("\taddi sp, sp, 8\n");
	// Depth--;
}

static void NnodePrint(uni::Nnode& nnod, stduint nest) {
	uni::Console.OutFormat("#");// GAS Style
	for0(i, nest) uni::Console.OutFormat(i + 1 == _LIMIT ? "->" : "--");
	uni::Console.OutFormat("|%s|\n", nnod.offs);
}

bool GetDirection(uni::Nnode* nnod) {
	//if (nnod->getParent() &&\
		nnod->getParent()->type == tok_func &&\
		!StrCompare(nnod->getParent()->addr, "OP@ASSIGN")) return true;
	return false;// true for LEFT-RIGHT
}

static void NnodeWalk(uni::Nnode* nnod)
{
	using namespace uni;
	Nnode* tmpnode = nnod;
	static stdsint val = 0;
	if (!nnod) return;
	bool direction = GetDirection(nnod);// true for LEFT-RIGHT
	if (!direction) while (tmpnode->next) tmpnode = tmpnode->next;
	while (tmpnode) {
		if (tmpnode->subf) NnodeWalk(tmpnode->subf);
		Console.OutFormat("# %s #", tmpnode->addr);
		tmpnode = direction ? tmpnode->next : tmpnode->getLeft();
	}
}

Variable variables;

static bool NnodeProcess(uni::Nnode* nnod, uni::Nchain* nchan)
{
	using namespace uni;
	Nnode* tmpnode = nnod;
	Tnode tn = { 0 };
	static stdsint val = 0;
	if (!nnod || !nchan) return true;
	bool direction = GetDirection(nnod);// true for LEFT-RIGHT

	if (!direction) while (tmpnode->next) {
		if (tmpnode->type == tok_symbol) {// have next
			tn.col = ((mag_node_t*)getExfield(*tmpnode))->col;
			tn.col = ((mag_node_t*)getExfield(*tmpnode))->row;
			mag_erro(arg_v[1], __LINE__, &tn, "unsupported symbol %s", tmpnode->addr);
			return false;
		}
		else tmpnode = tmpnode->next;
	}
	while (tmpnode) {
		if (tmpnode->subf && !NnodeProcess(tmpnode->subf, nchan)) return false;
		mag_node_t* magnod;
		// assume it is 1&2 tree
		switch (tmpnode->type) {
		case tok_number:
			val = atoins(tmpnode->addr);
			dst->OutFormat("\tli a0, %[i]\n", val);
			if (tmpnode->getLeft()) {// right
				push();
			}
			else if (tmpnode->pare && tmpnode->next) {// left
				pop("a1");
			}
			break;
		case tok_func:
			if (tmpnode->addr) {
				magnod = (mag_node_t*)getExfield(*tmpnode);
				asserv(magnod->bind)((Dchain*)tmpnode);
			else {
				tn.col = ((mag_node_t*)getExfield(*tmpnode))->col;
				tn.col = ((mag_node_t*)getExfield(*tmpnode))->row;
				mag_erro(arg_v[1], __LINE__, &tn, "bad operator %s", tmpnode->addr);
			}
			}
			if (tmpnode->getLeft()) {
				push();
			}
			else if (tmpnode->pare && tmpnode->next) { // getParent
				pop("a1");
			}
			break;
		case tok_identy:
			if (tmpnode->addr) {
				stdsint offset = variables.GetOffset(tmpnode->addr);
				dst->OutFormat("\taddi a0, fp, -%[i] # %s\n", offset, tmpnode->addr);// leave the offset
				if (tmpnode->getParent() && tmpnode->getParent()->addr && !!StrCompare(tmpnode->getParent()->addr, "OP@ASSIGN")) {
					dst->OutFormat("\tld a0, 0(a0)\n", offset, tmpnode->addr);
				}
				if (tmpnode->getLeft()) {// right
					push();
				}
				else if (tmpnode->pare && tmpnode->next) {// left
					pop("a1");
				}
			}
			break;
		default:
			tn.col = ((mag_node_t*)getExfield(*tmpnode))->col;
			tn.col = ((mag_node_t*)getExfield(*tmpnode))->row;
			mag_erro(arg_v[1], __LINE__, &tn, "bad node type %d", tmpnode->type);
			return false;
		}


		if (!direction) tmpnode = tmpnode->getLeft();
		else tmpnode = tmpnode->next;
	}
	return true;
}





int magic(int argc, char** argv) {
	int state = 0;
	using namespace uni;
	MagInn minn;
	LinearParser parser(minn);
	Dchain dc;
	NodeChain operators(NodeHeapFreeSimple);
	HostFile* output_file = nullptr;
	bool emit_ir = false;
	rostr output_path = nullptr;
	Vector<rostr> inputs;
	for (int i = 1; i < argc; i++) {
		if (!StrCompare(argv[i], "--emit-ir") || !StrCompare(argv[i], "-emit-ir")) {
			emit_ir = true;
		}
		else if (!StrCompare(argv[i], "-o")) {
			if (++i >= argc) {
				plogerro("expected filename after -o");
				return 1;
			}
			output_path = argv[i];
		}
		else inputs.Append(argv[i]);
	}


	dst = &Console;
	src = &minn;


	if (!dst || !src) {
		plogerro("expected input and output streams.");
		state = 1;
		goto endo;
	}

	if (inputs.isEmpty()) {
		ploginfo("Usage: mgc [--emit-ir] expression... [-o output]");
		state = 0;
		goto endo;
	}

	if (output_path) {
		output_file = new HostFile(output_path, FileOpenType::Write);
		if (!*output_file) {
			plogerro("cannot open output file %s", output_path);
			delete output_file;
			return 1;
		}
		dst = output_file;
	}

	if (!emit_ir) {
		dst->OutFormat("%s%s", _AUT_HEAD[platform], _AUT_ARCHITECT[platform]);
		dst->OutFormat("%s", _ASM_GLOBAL[platform]); // GLOBAL main
		dst->OutFormat("#%s\n", inputs[0]);
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

		auto netroot = npu.GetNetwork()->Root();

		npu.ParseStatements_CPL();
		if (emit_ir) {
			clover::Module module;
			if (!clover::Generate(*fileScope.GetNetwork(), module) || !clover::Print(module, *dst)) {
				if (output_file) delete output_file;
				return 1;
			}
			if (output_file) delete output_file;
			return 0;
		}
		variables.List(fileScope.GetNetwork());

		dst->OutFormat("main:\n");
		// Procedure Prologue
		dst->OutFormat("\taddi sp, sp, -8\n");
		dst->OutFormat("\tsd fp, 0(sp)\n");
		dst->OutFormat("\tmv fp, sp\n");
		dst->OutFormat("\taddi sp, sp, -%[u]\n", variables.GetTotalSize());
		dst->OutFormat("main_1:\n");


		NnodeWalk(netroot); dst->OutFormat("\n");
		npu.GetNetwork()->Traversal(NnodePrint);

		for (auto crt = npu.GetNetwork()->Root(); crt; crt = crt->next) {
			if (crt->type != tok_statement) {
				plogerro("Bad Statement");
				return 1;
			}
			dst->OutFormat("\n");
			if (crt->subf) NnodeProcess(crt->subf, npu.GetNetwork());
		}

		// for only 1 func:
		variables.~Variable();

	}

	// Procedure Epilogue
	dst->OutFormat("main_endo:\n");
	dst->OutFormat("\tmv sp, fp\n");
	dst->OutFormat("\tld fp, 0(sp)\n");
	dst->OutFormat("\taddi sp, sp, 8\n");
	dst->OutFormat("\t");
	dst->OutFormat(_ASM_RET[platform]);// Normal but Interrupt ...


endo:
	if (output_file) delete output_file;

	return state;
}

const char* mgc_top_file_path = nullptr;

int main(int argc, char** argv)
{
	using namespace uni;
	arg_v = argv;
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
