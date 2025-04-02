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
#include "../inc/cpp/trait/XstreamTrait.hpp"
#include <stdio.h>

_ESYM_C long int strtol(const char *str, char **endptr, int base);

class MagInn : public uni::IstreamTrait {
	char* p;
public:
	MagInn(char* p) : p(p) {}
	virtual int inn() {
		return *p ? *p++ : -1;
	}

};

enum Architecture_t platform = Architecture_RISCV64;
// AASM Architecture_x86     
// GAS  Architecture_RISCV64

//[Update] Accept + and - operators

int magic(int argc, char** argv) {
	using namespace uni;
	OstreamTrait* dst;// to be a HostFile or ConsoleLine
	IstreamTrait* src;
	MagInn minn(argv[1]);
	LinearParser parser(minn);
	Dchain dc;
	dst = &Console;
	src = &minn;

	if (!dst || !src) {
		plogerro("expected input and output streams.");
		return 1;
	}

	if (argc != 2) {
		plogerro("%s: invalid count of arguments\n", argv[0]);
		return 1;
	}

	dst->OutFormat("%s%s", _AUT_HEAD[platform], _AUT_ARCHITECT[platform]);
	dst->OutFormat("%s", _ASM_GLOBAL[platform]); // GLOBAL main
	dst->OutFormat("main:\n");

	parser.Parse(dc);
	// FORM: num (op num) (op num)...

	auto nod = dc.Root()->next;
	if (true) {
		bool neg;
		stdsint val = 0;
		dst->OutFormat("\t");
		dst->OutFormat(_ASM_MOV[platform], atoins(nod->addr));
		while (nod = nod->next) switch (nod->type) {
		case tok_symbol:
			break;
		case tok_number:
			if (!StrCompare(nod->left->addr, "+")) neg = false;
			else if (!StrCompare(nod->left->addr, "-")) neg = true;
			else {
				plogerro("do number: invalid grammar '%s'\n", nod->left->addr);
				return 1;
			}
			val = atoins(nod->addr);
			if (neg) val = -val;
			dst->OutFormat("\t");
			dst->OutFormat(_ASM_ADD[platform], val);
			break;
		default:
			plogerro("invalid nodtype '%s'\n", nod->addr);
			break;
		}
	}

	dst->OutFormat("\t");
	dst->OutFormat(_ASM_RET[platform]);
	return 0;
}

int main(int argc, char** argv)
{
	using namespace uni;
	//Letvar(buf, char*, malc(0x1000));

	if (int stat = magic(argc, argv)) {
		plogerro("Bad %d", stat);
		goto endo;
	}
endo:
	//memf(buf);
	return malc_count;
}
