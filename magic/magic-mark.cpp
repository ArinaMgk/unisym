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
//       |-> stdout
//       |-> Unix Manual Page

#include "magice.hpp"
#include "../inc/c/file.h"
#include "include/markproc.hpp"

using namespace uni;



OstreamTrait* out;
static void NnodeWalk(uni::Nnode* nnod, stduint level = 0)
{
	Nnode* tmpnode = nnod;
	if (!nnod) return;
	while (tmpnode) {
		if (level) for0(i, level) out->OutFormat("-> ");
		else out->OutFormat("---");
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
		out->OutFormat("\n");
		if (tmpnode->subf) {
			
			NnodeWalk(tmpnode->subf, level + 1);
		}
		tmpnode = tmpnode->next;
	}
}
void DnodeHeapFreeMagice(pureptr_t offs) {
	Letvar(obj, uni::Dnode*, offs);
	// ploginfo(": %s %s", obj->addr, ((TagObject<mgc_datatype>*)obj->type)->offs);
	mfree(obj->addr);// iden
	mfree(((TagObject<mgc_datatype>*)obj->type)->offs);
	mfree(obj->type);// IdenObject
}

bool mark_debug = false;

//{issue}!!! why level-0 are not only one
///her/unisym/magic$ ./makmgc.sh && mgc /her/unisym/demo/test/mark/doc0/doc0.mgc
int mark(int argc, char** argv) {
	//ploginfo("process %s into .tex, .md, .html", argv[1]);
	String filename;
	String out_filename;
	MarkProcessor::TextFormat out_fmt = MarkProcessor::TextFormat::None;

	for (int i = 1; i < argc; i++) {
		if (!StrCompare(argv[i], "-f") && i + 1 < argc) {
			i++;
			if (!StrCompare(argv[i], "html")) out_fmt = MarkProcessor::TextFormat::HTML;
			else if (!StrCompare(argv[i], "tex")) out_fmt = MarkProcessor::TextFormat::Tex;
			else if (!StrCompare(argv[i], "md")) out_fmt = MarkProcessor::TextFormat::Markdown;
			else if (!StrCompare(argv[i], "stdout")) out_fmt = MarkProcessor::TextFormat::STDOUT;
		}
		else if (!StrCompare(argv[i], "-o") && i + 1 < argc) {
			i++;
			out_filename = argv[i];
		}
		else if (argv[i][0] != '-') {
			filename = argv[i];
		}
	}

	if (out_fmt == MarkProcessor::TextFormat::None) {
		if (out_filename.reference()) {
			stduint len = StrLength(out_filename.reference());
			if (len >= 5 && !StrCompare(out_filename.reference() + len - 5, ".html")) out_fmt = MarkProcessor::TextFormat::HTML;
			else if (len >= 4 && !StrCompare(out_filename.reference() + len - 4, ".tex")) out_fmt = MarkProcessor::TextFormat::Tex;
			else if (len >= 3 && !StrCompare(out_filename.reference() + len - 3, ".md")) out_fmt = MarkProcessor::TextFormat::Markdown;
			else out_fmt = MarkProcessor::TextFormat::STDOUT;
		} else {
			out_fmt = MarkProcessor::TextFormat::STDOUT;
		}
	}

	if (out_filename.getByteCount() == 0 && out_fmt != MarkProcessor::TextFormat::STDOUT) {
		if (out_fmt == MarkProcessor::TextFormat::HTML) {
			char* tmp = StrReplace(filename.reference(), ".mgc", ".mgc.html", NULL);
			out_filename = tmp;
			mfree(tmp);
		} else if (out_fmt == MarkProcessor::TextFormat::Tex) {
			char* tmp = StrReplace(filename.reference(), ".mgc", ".mgc.tex", NULL);
			out_filename = tmp;
			mfree(tmp);
		} else if (out_fmt == MarkProcessor::TextFormat::Markdown) {
			char* tmp = StrReplace(filename.reference(), ".mgc", ".mgc.md", NULL);
			out_filename = tmp;
			mfree(tmp);
		}
	}

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

	// divide into statements by ';'
	NestedParseUnit npu(dc, 0);
	npu.GetNetwork()->func_free = NnodeHeapFreeSimple;
	npu.ParseStatements_CPL();
	npu.ParseParen(npu.GetNetwork()->Root(), false);

	MarkProcessor* proc = nullptr;
	HostFile* outfile = nullptr;

	if (out_fmt == MarkProcessor::TextFormat::HTML) {
		outfile = new HostFile(out_filename.reference(), FileOpenType::Write);
		proc = new ProcessorHTML(outfile);
	} else if (out_fmt == MarkProcessor::TextFormat::Tex) {
		outfile = new HostFile(out_filename.reference(), FileOpenType::Write);
		proc = new ProcessorTex(outfile);
	} else if (out_fmt == MarkProcessor::TextFormat::Markdown) {
		outfile = new HostFile(out_filename.reference(), FileOpenType::Write);
		proc = new ProcessorMarkdown(outfile);
	} else {
		proc = new ProcessorStdout(&Console);
	}

	proc->variables.chn.func_comp = (_tocomp_ft)StrCompare;
	proc->variables.chn.refChain().func_free = DnodeHeapFreeMagice;

	for (auto nod = npu.GetNetwork()->Root(); nod; nod = nod->next) {
		for (auto nnod = nod->subf; nnod; nnod = nnod->next) proc->proc(nnod);
	}

	delete proc;
	if (outfile) delete outfile;
	

	// NnodeWalk(npu.GetNetwork()->Root());

	// ---- END ----
	// char* _Heap texname = StrReplace(filename.reference(), ".mgc", ".mgc.tex", NULL);
	// ploginfo(".tex \t=> %s", texname);
	// char* _Heap mdname = StrReplace(filename.reference(), ".mgc", ".mgc.md", NULL);
	// ploginfo(".md  \t=> %s", mdname);
	// char* _Heap htmlname = StrReplace(filename.reference(), ".mgc", ".mgc.html", NULL);
	// ploginfo(".html\t=> %s", htmlname);
	// mfree(texname);
	// mfree(mdname);
	// mfree(htmlname);


	return nil;
}

