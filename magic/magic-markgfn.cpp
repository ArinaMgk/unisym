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
		txt = (rostr)proc->variables[n->addr]->offs;
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
		//proc->out("<h", 2);
		//proc->out(dc[0]->addr, StrLength(dc[0]-/>addr));
		//proc->out(">", 1);
		//proc->out(title, StrLength(title));
		//proc->out("</h", 3);
		//proc->out(dc[0]->addr, StrLength(dc[0]-/>addr));
		//proc->out(">\n", 2);
		break;
	case MarkProcessor::TextFormat::Tex:
		//{TODO}
		break;
	case MarkProcessor::TextFormat::Markdown:
	case MarkProcessor::TextFormat::STDOUT:
		for0(i, atoins(dc[0]->addr)) proc->OutChar('#');
		proc->OutFormat(" %s\n", title);
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
		proc->OutFormat(" %s\n", txt);
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



