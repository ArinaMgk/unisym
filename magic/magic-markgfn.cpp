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
rostr SeekString(uni::Dnode* n, MarkProcessor* proc)
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
			proc->OutFormat("<h%d>%s</h%d>", level, title, level);
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
			if (proc->fmt.M) {
				proc->fmt.M = false;
				proc->fmt_valid = false;
			}
			if (proc->fmt.T) {
				proc->fmt.T = false;
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
		case 'M':
			if (!proc->fmt.M) {
				proc->fmt.M = true;
				proc->fmt_valid = false;
			}
			break;
		case 'm':
			if (proc->fmt.M) {
				proc->fmt.M = false;
				proc->fmt_valid = false;
			}
			break;
		case 'T':
			if (!proc->fmt.T) {
				proc->fmt.T = true;
				proc->fmt_valid = false;
			}
			break;
		case 't':
			if (proc->fmt.T) {
				proc->fmt.T = false;
				proc->fmt_valid = false;
			}
			break;

		default:
			plogwarn("Format: Unknown format specifier ^%c", chh);
			break;
		}
	}
	break;
	case '\n':
		{
			extern bool TableEngineActive();
			if (TableEngineActive()) {
				if (proc->txtfmt == MarkProcessor::TextFormat::Tex) {
					proc->OutFormat("\\\\");
				} else if (proc->txtfmt == MarkProcessor::TextFormat::HTML) {
					proc->OutFormat("<br>\n");
				} else {
					proc->OutFormat(" ");
				}
			} else {
				proc->OutChar('\n');
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

extern const char* mgc_top_file_path;

static char* include_stack[64];
static int include_depth = 0;

uni::String ResolveIncludePath(const char* path) {
	if (!path || !path[0]) return uni::String("");
	if (path[0] == '/' || path[0] == '\\' || (path[0] && path[1] == ':')) {
		return uni::String(path);
	}
	const char* base = nullptr;
	if (include_depth > 0) base = include_stack[include_depth - 1];
	else if (mgc_top_file_path) base = mgc_top_file_path;
	else return uni::String(path);
	
	int last_slash = -1;
	for (int i = 0; base[i]; i++) {
		if (base[i] == '/' || base[i] == '\\') last_slash = i;
	}
	if (last_slash != -1) {
		uni::String res;
		for (int i = 0; i <= last_slash; i++) res << base[i];
		for (int i = 0; path[i]; i++) res << path[i];
		return res;
	}
	return uni::String(path);
}

void GF_TexSubimport(uni::Dchain* chain, MarkProcessor* proc)
{
	using namespace uni;
	rostr dir, file;
	if (chain->Count() == 2) {
		dir = SeekString((*chain)[0], proc);
		file = SeekString((*chain)[1], proc);
		if (dir && file) {
			if (proc->txtfmt == MarkProcessor::TextFormat::Tex) {
				proc->OutFormat("\\subimport{%s}{%s}\n", dir, file);
			}
		}
	} else {
		plogerro("TexSubimport: Missing or invalid arguments");
	}
}

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

		uni::String resolved_path = ResolveIncludePath(txt);
		const char* resolved_cstr = resolved_path.reference();

		for (int i = 0; i < include_depth; i++) {
			if (!StrCompare(include_stack[i], resolved_cstr)) {
				plogerro("Include: Cyclic inclusion of %s", resolved_cstr);
				return;
			}
		}
		if (include_depth >= 64) {
			plogerro("Include: Max depth exceeded");
			return;
		}
		include_stack[include_depth] = (char*)malc(StrLength(resolved_cstr) + 1);
		StrCopy(include_stack[include_depth], resolved_cstr);
		include_depth++;

		HostFile file(resolved_cstr);
		if (!file) {
			plogerro("Include: Cannot open %s", resolved_cstr);
			include_depth--;
			memf(include_stack[include_depth]);
			include_stack[include_depth] = nullptr;
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

		HostFile file2(resolved_cstr);
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
		memf(include_stack[include_depth]);
		include_stack[include_depth] = nullptr;
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

void GF_Inline(uni::Dchain* chain, MarkProcessor* proc)
{
	using namespace uni;
	rostr target, txt;
	if (chain->Count() == 2) {
		target = SeekString((*chain)[0], proc);
		txt = SeekString((*chain)[1], proc);
		if (target && txt) {
			bool match = false;
			if (proc->txtfmt == MarkProcessor::TextFormat::Tex && (!StrCompare(target, "tex") || !StrCompare(target, "latex"))) match = true;
			else if (proc->txtfmt == MarkProcessor::TextFormat::HTML && (!StrCompare(target, "html") || !StrCompare(target, "htm"))) match = true;
			else if (proc->txtfmt == MarkProcessor::TextFormat::Markdown && (!StrCompare(target, "md") || !StrCompare(target, "markdown"))) match = true;
			else if (proc->txtfmt == MarkProcessor::TextFormat::STDOUT && !StrCompare(target, "stdout")) match = true;

			if (match) {
				proc->OutFormat("%s\n", txt);
			}
		}
	} else {
		plogerro("Inline: Missing or invalid arguments");
	}
}
