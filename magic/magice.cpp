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

#include "./magice.hpp"


#include "../inc/c/uctype.h"

#include "../inc/cpp/parse.hpp"
#include "../inc/c/compile/asmcode.h"

uni::NodeChain* ele_stack;

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


	dst = &Console;
	src = &minn;
	ele_stack = new NodeChain(NULL);

	if (!dst || !src) {
		plogerro("expected input and output streams.");
		state = 1;
		goto endo;
	}

	if (argc <= 1) {
		ploginfo("Usage: magic str0 (str1) (str2 ...)\n\t Magice will combine them.");
		state = 0;
		goto endo;
	}


	dst->OutFormat("%s%s", _AUT_HEAD[platform], _AUT_ARCHITECT[platform]);
	dst->OutFormat("%s", _ASM_GLOBAL[platform]); // GLOBAL main
	dst->OutFormat("#%s\n", argv[1]);

	parser.GHT = false;
	for1(i, argc - 1) {
		minn.p = argv[i];
		parser.Parse(dc);
	}

	{
		Operators::List(operators);

		NestedParseUnit fileScope(dc, &operators, sizeof(mag_node_t));
		NestedParseUnit& npu = fileScope;

		if (!npu.Parse()) return 1;

		auto netroot = npu.GetNetwork()->Root();

		npu.ParseStatements_CPL();
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

	delete ele_stack;
	return state;
}

int main(int argc, char** argv)
{
	using namespace uni;
	arg_v = argv;
	//Letvar(buf, char*, malc(0x1000));
	int stat = 0;

	FILE* file = 0;

	if (argc > 1 && 0) {
		// judge "mgc XXX.mgc"

		stat;
	}
	else stat = magic(argc, argv);
	//memf(buf);
	if (malc_count) plogerro("Memory Leak %[u]", malc_count);
	return stat;
}
