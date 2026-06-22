// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Compile) Mark Layer
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
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

#include "include/markproc.hpp"

extern bool mark_debug;

static mgc_datatype GetType(rostr str) {
	if (!StrCompare("Number", str)) return mgc_datatype::Number;
	if (!StrCompare("String", str)) return mgc_datatype::String;
	return mgc_datatype::None;
}

// NULL if not found
static rostr SeekString(uni::Dnode* n, MarkProcessor* proc)
{
	rostr txt = nullptr;
	if (n->type == tok_identy) {
		//{} chk type string then:
		txt = proc->variables[n->addr] ? (rostr)proc->variables[n->addr]->offs : NULL;
	}
	else if (n->type == tok_string) {
		txt = n->addr;
	}
	return txt;
}

void GF_Set(uni::Dchain* chain, MarkProcessor* proc)
{
	using namespace uni;
	if (chain->Count() == 3); else {
		// plogerro
		return;
	}
	Dchain& dc = *chain;
	proc->variables.Modify(dc[0]->addr, StrHeap(dc[2]->addr), GetType(dc[1]->addr), true);
	if (mark_debug) ploginfo("Set(%s %s %s)", dc[0]->addr, dc[1]->addr, dc[2]->addr);
}

void GF_Append(uni::Dchain* chain, MarkProcessor* proc)
{
	//{TEMP} Omit En lango suffix
	using namespace uni;
	if (chain->Count() == 3); else {
		// plogerro
		return;
	}
	Dchain& dc = *chain;
	String str = dc[0]->addr;
	str += ":";
	str += dc[1]->addr;
	auto obj = proc->variables[dc[0]->addr];
	proc->variables.Modify(str.reference(), StrHeap(dc[2]->addr), obj->type, true);
	if (mark_debug) ploginfo("Append(%s:%s = %s)", dc[0]->addr, dc[1]->addr, dc[2]->addr);
}

void GF_Title(uni::Dchain* chain, MarkProcessor* proc)
{
	using namespace uni;
	if (chain->Count() == 2); else {
		// plogerro
		return;
	}
	Dchain& dc = *chain;
	//{TEMP} only En
	rostr title;
	if (title = SeekString(dc[1], proc));
	else {
		// plogerro
		return;
	}
	switch (proc->txtfmt) {
	case MarkProcessor::TextFormat::HTML:
		{
			int level = atoins(dc[0]->addr);
			if (level > 6) level = 6;
			proc->OutFormat("<h%d>%s</h%d>\n", level, title, level);
		}
		break;
	case MarkProcessor::TextFormat::Tex:
		proc->OutFormat("\n");
		switch (atoins(dc[0]->addr)) {
		case 1: proc->OutFormat("\\chapter{%s}\n", title); break;
		case 2: proc->OutFormat("\\section{%s}\n", title); break;
		case 3: proc->OutFormat("\\subsection{%s}\n", title); break;
		case 4: proc->OutFormat("\\subsubsection{%s}\n", title); break;
		case 5: proc->OutFormat("\\paragraph{%s}\n", title); break;
		case 6: proc->OutFormat("\\subparagraph{%s}\n", title); break;
		case 7: proc->OutFormat("\\subsubparagraph{%s}\n", title); break;
		default: proc->OutFormat("\\textbf{%s}\n", title); break;
		}
		break;
	case MarkProcessor::TextFormat::Markdown:
	case MarkProcessor::TextFormat::STDOUT:
		proc->OutChar('\n');
		for0(i, atoins(dc[0]->addr)) proc->OutChar('#');
		proc->OutFormat(" %s\n\n", title);
		break;
	default:
		break;
	}
}

void GF_Out(uni::Dchain* chain, MarkProcessor* proc)
{
	using namespace uni;
	rostr txt;
	for (auto n = chain->Root(); n; n = n->next) {
		if (txt = SeekString(n, proc));
		else {
			// plogerro
			return;
		}
		proc->OutFormat("%s\n", txt);
	}
}

void GF_Format(uni::Dchain* chain, MarkProcessor* proc)
{
	using namespace uni;
	Dnode* fnod_mt = chain->Root();
	char* p = fnod_mt->addr, ch;
	Dnode* n = fnod_mt->next;
	rostr txt;
	//
	while (ch = *p++) switch (ch) {
	case '%':
	{
		char chh = *p++;
		switch (chh) {
		case 's':
			if (!n) {
				plogerro("Format: Missing argument");
				return;
			}
			proc->OutFormat("%s", SeekString(n, proc));
			n = n->next;
			break;
		default:
			plogwarn("Format: Unknown format specifier %%%c", chh);
			break;
		}
	}
	break;
	case '^':
	{
		char chh = *p++;
		switch (chh) {
		case '^':// B I U
			if (proc->fmt.B) {
				proc->fmt.B = false;
				proc->fmt_valid = false;
			}
			if (proc->fmt.I) {
				proc->fmt.I = false;
				proc->fmt_valid = false;
			}
			if (proc->fmt.U) {
				proc->fmt.U = false;
				proc->fmt_valid = false;
			}
			break;
		case 'B':
			if (!proc->fmt.B) {
				proc->fmt.B = true;
				proc->fmt_valid = false;
			}
			break;
		case 'b':
			if (proc->fmt.B) {
				proc->fmt.B = false;
				proc->fmt_valid = false;
			}
			break;
		case 'I':
			if (!proc->fmt.I) {
				proc->fmt.I = true;
				proc->fmt_valid = false;
			}
			break;
		case 'i':
			if (proc->fmt.I) {
				proc->fmt.I = false;
				proc->fmt_valid = false;
			}
			break;
		case 'U':
			if (!proc->fmt.U) {
				proc->fmt.U = true;
				proc->fmt_valid = false;
			}
			break;
		case 'u':
			if (proc->fmt.U) {
				proc->fmt.U = false;
				proc->fmt_valid = false;
			}
			break;

		default:
			plogwarn("Format: Unknown format specifier ^%c", chh);
			break;
		}
	}
	break;
	default:
		proc->OutChar(ch);
		break;
	}
}

void GF_Picture(uni::Dchain* chain, MarkProcessor* proc)
{
	using namespace uni;
	rostr txt;
	for (auto n = chain->Root(); n; n = n->next) {
		if (txt = SeekString(n, proc));
		else {
			// plogerro
			return;
		}
		switch (proc->txtfmt) {
		case MarkProcessor::TextFormat::HTML:
			proc->OutFormat("<img src=\"%s\" alt=\"image\" />\n", txt);
			break;
		case MarkProcessor::TextFormat::Tex:
			proc->OutFormat("\\begin{figure}[htbp]\n\\centering\n\\includegraphics[width=\\textwidth]{%s}\n\\end{figure}\n", txt);
			break;
		case MarkProcessor::TextFormat::Markdown:
			proc->OutFormat("![](%s)\n", txt);
			break;
		case MarkProcessor::TextFormat::STDOUT:
			proc->OutFormat("🖼️（%s）\n", txt);
			break;
		default:
			break;
		}
		
	}
}

#include "../inc/c/file.h"

static const char* include_stack[64];
static int include_depth = 0;

void GF_Include(uni::Dchain* chain, MarkProcessor* proc)
{
	using namespace uni;
	rostr txt;
	for (auto n = chain->Root(); n; n = n->next) {
		if (txt = SeekString(n, proc));
		else {
			plogerro("Include: invalid path argument.");
			return;
		}

		for (int i = 0; i < include_depth; i++) {
			if (!StrCompare(include_stack[i], txt)) {
				plogerro("Include: Cyclic inclusion of %s", txt);
				return;
			}
		}
		if (include_depth >= 64) {
			plogerro("Include: Max depth exceeded");
			return;
		}
		include_stack[include_depth++] = txt;

		HostFile file(txt);
		if (!file) {
			plogerro("Include: Cannot open %s", txt);
			include_depth--;
			continue;
		}

		bool has_sig = false;
		String buf(String::Charset::UTF8, 22);
		for (stduint i = 0; i < 21; i++) {
			int ch = file.inn();
			if (ch == EOF) break;
			else buf << ch;
		}
		if (!StrCompare(buf.reference(), "#UTF-8 MAgicRK(1) . .")) {
			has_sig = true;
		}

		HostFile file2(txt);
		if (has_sig) {
			for (stduint i = 0; i < 21; i++) file2.inn();
		}

		Dchain dc;
		LinearParser parser(file2);
		parser.GHT = false;
		parser.method_omit_comment = true;
		parser.handler_comment = LineParse_Comment_Cpp;
		parser.method_string_double_quote = true;
		parser.method_string_escape_sequence = true;
		parser.Parse(dc);

		NestedParseUnit npu(dc, 0);
		npu.GetNetwork()->func_free = NnodeHeapFreeSimple;
		npu.ParseStatements_CPL();
		npu.ParseParen(npu.GetNetwork()->Root(), false);

		for (auto nod = npu.GetNetwork()->Root(); nod; nod = nod->next) {
			for (auto nnod = nod->subf; nnod; nnod = nnod->next) {
				proc->proc(nnod);
			}
		}

		include_depth--;
	}
}

void GF_IncludeWeak(uni::Dchain* chain, MarkProcessor* proc)
{
	using namespace uni;
	rostr txt;
	for (auto n = chain->Root(); n; n = n->next) {
		if (txt = SeekString(n, proc));
		else {
			plogerro("IncludeWeak: invalid path argument.");
			return;
		}

		switch (proc->txtfmt) {
		case MarkProcessor::TextFormat::HTML:
			proc->OutFormat("<a href=\"%s\">%s</a>\n", txt, txt);
			break;
		case MarkProcessor::TextFormat::Tex:
			proc->OutFormat("\\input{%s}\n", txt);
			break;
		case MarkProcessor::TextFormat::Markdown:
			proc->OutFormat("[%s](%s)\n", txt, txt);
			break;
		case MarkProcessor::TextFormat::STDOUT:
			proc->OutFormat("\n[Link: %s]\n", txt);
			break;
		default:
			break;
		}
	}
}
