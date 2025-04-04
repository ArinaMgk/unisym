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

enum Architecture_t platform = Architecture_RISCV64;
// AASM Architecture_x86     
// GAS  Architecture_RISCV64

//[Update] Accept + and - operators

void mag_erro(rostr srcfile, stduint lineno, uni::Tnode* dn, rostr fmt, ...) {
	using namespace uni;
	Letpara(plist, fmt);
	printlogx(_LOG_ERROR, fmt, plist);
	//
	Console.OutFormat("%s\n", srcfile);
	for0(i, dn->col - 1) Console.OutFormat(" ");
	Console.OutFormat("^\n");
}

int magic(int argc, char** argv) {
	using namespace uni;
	OstreamTrait* dst;// to be a HostFile or ConsoleLine
	IstreamTrait* src;
	MagInn minn;
	LinearParser parser(minn);
	Dchain dc;
	dst = &Console;
	src = &minn;

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

	if (Tnode* nod = (Tnode*)dc.Root()) {
		bool neg = false;
		stdsint val = 0;
		dst->OutFormat("\t");
		dst->OutFormat(_ASM_MOV[platform], 0);
		do switch (nod->type) {
		case tok_symbol:
			if (!StrCompare(nod->addr, "+")) neg = false;
			else if (!StrCompare(nod->addr, "-")) neg = true;
			else {
				mag_erro(argv[1] _TEMP, 1, nod, "bad symbol %s", nod->addr);
				return 1;
			}
			break;
		case tok_number:
			val = atoins(nod->addr);
			if (neg) val = -val;
			dst->OutFormat("\t");
			dst->OutFormat(_ASM_ADD[platform], val);
			break;
		default:
			mag_erro(argv[1] _TEMP, 1, nod, "bad nodtype '%s'", nod->addr);
			break;
		} while (nod = nod->next);
	}

	dst->OutFormat("\t");
	dst->OutFormat(_ASM_RET[platform]);
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
