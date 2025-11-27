// ASCII C++ TAB4 LF
// Docutitle: Magice Mark-Like Language Compiler
// Input    : Intermediate mark-language
// Output   : HTML, LaTeX, Markdown, ...
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

// .mgc ---> .html
//       |-> .tex
//       |-> .md

#include "magice.hpp"
#include "../inc/c/file.h"
#include "include/markproc.hpp"

using namespace uni;

OstreamTrait* out;
static void NnodeWalk(uni::Nnode* nnod, stduint level = 0)
{
	Nnode* tmpnode = nnod;
	if (!nnod) return;
	for0(i, level) out->OutFormat("-> ");
	while (tmpnode) {
		if (tmpnode->subf) {
			out->OutFormat("\n");
			NnodeWalk(tmpnode->subf, level + 1);
		}
		if (tmpnode->addr) {
			if (tmpnode->type == tok_identy) {
				out->OutFormat(ConForecolor "%s" ConForecolor, CON_FORE_YELLOW, tmpnode->addr, 0);
			}
			else if (tmpnode->type == tok_string) {
				out->OutFormat(ConForecolor "%s" ConForecolor, CON_FORE_GREEN, tmpnode->addr, 0);
			}
			else if (tmpnode->type == tok_number) {
				out->OutFormat(ConForecolor "%s" ConForecolor, CON_FORE_CYAN, tmpnode->addr, 0);
			}
			else out->OutFormat(" %s ", tmpnode->addr);
		}
		tmpnode = tmpnode->next;
	}
}

enum class datatype {
	Number,// double
	String,
};

///her/unisym/magic$ ./makmgc.sh && mgc /her/unisym/demo/test/mark/doc0/doc0.mgc
int mark(int argc, char** argv) {
	ploginfo("process %s into .tex, .md, .html", argv[1]);
	String filename = argv[1];
	HostFile file(filename.reference());
	byte ch;

	out = &Console;

	Dchain dc;
	LinearParser parser(file);
	parser.GHT = false;
	for0(i, sizeof("#UTF-8 MAgicRK(1) . ."))
		(void)file.inn();// skip
	parser.method_omit_comment = true;
	parser.handler_comment = LineParse_Comment_Cpp;
	parser.method_string_double_quote = true;
	parser.method_string_escape_sequence = true;
	parser.Parse(dc);

	//for(auto nod = dc.Root(); nod; nod = nod->next) out->OutFormat("%s\n", nod->addr);

	// divide into statements by ';'
	NestedParseUnit npu(dc, 0);
	npu.GetNetwork()->func_free = NnodeHeapFreeSimple;
	npu.ParseStatements_CPL();
	
	// Table of Variables
	// // str[en] -> "str"
	// // str[nh] -> "str:nh"
	// // str[cn] -> "str:cn"
	TagChain<datatype> variables;


	// END

	
	out->OutFormat("There are %u nodes\n", npu.GetNetwork()->Count());
	//auto nod = dc.Root();
	//while (nod) {
	//	if (nod->addr) out->OutFormat(nod->addr);
	//	nod = nod->next;
	//}
	NnodeWalk(npu.GetNetwork()->Root());
	out->OutChar('\n');
	char* _Heap texname = StrReplace(filename.reference(), ".mgc", ".mgc.tex", NULL);
	ploginfo(".tex \t=> %s", texname);
	char* _Heap mdname = StrReplace(filename.reference(), ".mgc", ".mgc.md", NULL);
	ploginfo(".md  \t=> %s", mdname);
	char* _Heap htmlname = StrReplace(filename.reference(), ".mgc", ".mgc.html", NULL);
	ploginfo(".html\t=> %s", htmlname);
	mfree(texname);
	mfree(mdname);
	mfree(htmlname);


	return nil;
}

