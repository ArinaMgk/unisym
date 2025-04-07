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

#include "../inc/cpp/unisym"
#include "../inc/c/uctype.h"
#include "../inc/c/consio.h"
#include "../inc/cpp/parse.hpp"
#include "../inc/c/compile/asmcode.h"
// #include "../inc/cpp/trait/XstreamTrait.hpp"
// #include <stdio.h>


class MagInn : public uni::IstreamTrait {
public:
	char* p;
	MagInn(char* p = 0) : p(p) {}
	virtual int inn() {
		return *p ? *p++ : -1;
	}

};

struct mag_node_t {
	stduint row, col;// same with tnode
	uni::_tok_bindfunc_t bind;
};

enum Architecture_t platform = Architecture_RISCV64;
// AASM Architecture_x86     
// GAS  Architecture_RISCV64

uni::TokenOperator
arimul{ "*","*",NULL },
aridiv{ "/","/",NULL },
ariadd{ "+","+",NULL },
arisub{ "-","-",NULL },
//
op_lev1[]{ arimul, aridiv },
op_lev2[]{ ariadd, arisub };

void mag_erro(rostr srcfile, stduint lineno, uni::Tnode* dn, rostr fmt, ...) {
	using namespace uni;
	Letpara(plist, fmt);
	printlogx(_LOG_ERROR, fmt, plist);
	//
	Console.OutFormat("%s\n", srcfile);
	if (dn->col) {
		for0(i, dn->col - 1) Console.OutFormat(" ");
		Console.OutFormat("^\n");
	}
}

#define togsym(a) a,numsof(a)

uni::NodeChain* ele_stack;

uni::OstreamTrait* dst;// to be a HostFile or ConsoleLine
uni::IstreamTrait* src;

char** arg_v;

static void push(void) {
	uni::Console.OutFormat("\taddi sp, sp, -8\n");
	uni::Console.OutFormat("\tsd a0, 0(sp)\n");
	// Depth++;
}
static void pop(rostr reg) {
	uni::Console.OutFormat("\tld %s, 0(sp)\n", reg);
	uni::Console.OutFormat("\taddi sp, sp, 8\n");
	// Depth--;
}

static void NnodePrint(const uni::Nnode* nnod, unsigned nest)
{
	uni::Nnode* crt = (uni::Nnode*)nnod;
	while (crt)
	{
		for0(i, nest) uni::Console.OutFormat(i + 1 == _LIMIT ? "->" : "--");
		uni::Console.OutFormat("%s\n", crt->offs);
		if (crt->subf) NnodePrint(crt->subf, nest + 1);
		crt = crt->next;
	}
}
static void NnodeProcess(uni::Nnode* nnod, uni::Nchain* nchan)
{
	using namespace uni;
	Nnode* tmpnode = nnod;
	Tnode tn = { 0 };
	static stdsint val = 0;
	while (tmpnode->next) {
		if (tmpnode->type == tok_symbol) {// have next
			if (tmpnode->next->type == tok_number) {
				if (!StrCompare("+", tmpnode->addr)) {
					srs(tmpnode->next->addr, StrHeapAppend("+", tmpnode->next->addr));
				}
				else if (!StrCompare("-", tmpnode->addr)) {
					srs(tmpnode->next->addr, StrHeapAppend("-", tmpnode->next->addr));
				}
			}
			else if (tmpnode->next->type == tok_func) {
			    tmpnode->next->addr[0] = tmpnode->addr[0];
			}
			else {
				tn.col = ((mag_node_t*)getExfield(tmpnode))->col;
				tn.col = ((mag_node_t*)getExfield(tmpnode))->row;
				mag_erro(arg_v[1], __LINE__, &tn, "bad symbol %s", tmpnode->addr);
			}
			tmpnode = nchan->Remove(tmpnode);
		}
		else tmpnode = tmpnode->next;
	}
	while (tmpnode) {
		if (tmpnode->subf) NnodeProcess(tmpnode->subf, nchan);
		// if (tmpnode->addr) Console.OutFormat("%s ", tmpnode->addr);

		// assume it is binary tree
		switch (tmpnode->type) {
		case tok_number:
			val = atoins(tmpnode->addr);
			dst->OutFormat("\t");
			dst->OutFormat("li a0, %[i]\n", val);
			if (tmpnode->getLeft()) {// right
				push();
			}
			else if (tmpnode->pare) {// left
				pop("a1");
			}
			break;
		case tok_func:
			if (!StrCompare("+", tmpnode->addr)) {
				dst->OutFormat("\t");
				dst->OutFormat("add a0, a0, a1\n");
			}
			else if (!StrCompare("-", tmpnode->addr)) {
				dst->OutFormat("\t");
				dst->OutFormat("sub a0, a0, a1\n");
			}
			else if (!StrCompare("*", tmpnode->addr)) {
				dst->OutFormat("\t");
				dst->OutFormat("mul a0, a0, a1\n");
			}
			else if (!StrCompare("/", tmpnode->addr)) {
				dst->OutFormat("\t");
				dst->OutFormat("div a0, a0, a1\n");
			}
			else {
				tn.col = ((mag_node_t*)getExfield(tmpnode))->col;
				tn.col = ((mag_node_t*)getExfield(tmpnode))->row;
				mag_erro(arg_v[1], __LINE__, &tn, "bad operator %s", tmpnode->addr);
			}
			if (tmpnode->getLeft()) {
				push();
			}
			else if (tmpnode->pare) {
				pop("a1");
			}
			break;
		default:
			tn.col = ((mag_node_t*)getExfield(tmpnode))->col;
			tn.col = ((mag_node_t*)getExfield(tmpnode))->row;
			mag_erro(arg_v[1], __LINE__, &tn, "bad node type %d", tmpnode->type);
			return;
		}


		tmpnode = tmpnode->getLeft();
	}
}



int magic(int argc, char** argv) {
	using namespace uni;
	arg_v = argv;
	MagInn minn;
	LinearParser parser(minn);
	Dchain dc;
	NodeChain operators(NodeHeapFreeSimple);
	dst = &Console;
	src = &minn;
	ele_stack = new NodeChain(NULL);

	if (!dst || !src) {
		plogerro("expected input and output streams.");
		return 1;
	}

	if (argc <= 1) {
		ploginfo("Usage: magic str0 (str1) (str2 ...)\n\t Magice will combine them.");
		return 0;
	}


	dst->OutFormat("%s%s", _AUT_HEAD[platform], _AUT_ARCHITECT[platform]);
	dst->OutFormat("%s", _ASM_GLOBAL[platform]); // GLOBAL main
	dst->OutFormat("main:\n");

	parser.GHT = false;
	for1(i, argc - 1) {
		minn.p = argv[i];
		parser.Parse(dc);
	}
	// FORM: num (op num) (op num)...

	// make operator list
	operators.Append(new TokenOperatorGroup(togsym(op_lev1), true), false);
	operators.Append(new TokenOperatorGroup(togsym(op_lev2), true), false);

	NestedParseUnit npu(dc, &operators, sizeof(mag_node_t));
	if (!npu.Parse()) return 1;

	auto netroot = npu.GetNetwork()->Root();
	// NnodePrint(netroot, 0);
	NnodeProcess(netroot, npu.GetNetwork());
	// NnodePrint(npu.GetNetwork()->Root(), 0);


	dst->OutFormat("\t");
	dst->OutFormat(_ASM_RET[platform]);
	delete ele_stack;
	return 0;
}

int main(int argc, char** argv)
{
	using namespace uni;
	//Letvar(buf, char*, malc(0x1000));
	int stat;
	stat = magic(argc, argv);
	//memf(buf);
	if (malc_count) plogerro("Memory Leak %[u]", malc_count);
	return stat;
}
