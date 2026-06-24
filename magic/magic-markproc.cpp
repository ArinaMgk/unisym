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

// GlobalFn( [ CommonFn( [ CommonFn/Val(...) ] ) ] ); GlobalFn(...) ...

struct gfnmap_entry { 
	rostr iden;
	void (*handler)(uni::Dchain* chain, MarkProcessor* proc);
};
static const gfnmap_entry gfnmap[] = {
	{"Set", GF_Set },
	{"Append", GF_Append },
	{"Title", GF_Title },
	{"Out", GF_Out },
	{"Format", GF_Format },
	{"Picture", GF_Picture },
	{"Include", GF_Include },
	{"IncludeWeak", GF_IncludeWeak },
	// Table:
	{"TableBegin", GF_TableBegin },
	{"TableEnd", GF_TableEnd },
	{"Row", GF_Row },
	{"RowHead", GF_RowHead },
	{"Rcell", GF_Rcell },
	{"RcellSkip", GF_RcellSkip },
	{"LineH", GF_LineH },
	{"TableCSV", GF_TableCSV },
	// List:
	{"ListBegin", GF_ListBegin },
	{"ListEnd", GF_ListEnd },
	{"ListItem", GF_ListItem },
};


static bool process(uni::Nnode* nod, MarkProcessor* thi) {
	uni::DnodeChain dchain;
	for (uni::Nnode* n = nod->subf; n; n = n->next) {
		if (n->type == tok_symbol) {
			//{} PASS and should be `,`
		}
		else {
			dchain.AppendHeapstr(n->addr)->type = n->type;
		}

	}
	for0a(i, gfnmap) {
		if (!StrCompare(nod->addr, gfnmap[i].iden)) {
			gfnmap[i].handler(&dchain, thi);
			return true;
		}
	}
	plogerro("MarkProcessor: Unknown Global Function `%s`.\n", nod->addr);
	return false;
}

extern bool TableEngineActive();

int ProcessorHTML::out(const char* str, stduint len) { 
	if (!fmt.B && !fmt.I && !fmt.U && !fmt.M) {
		for (stduint i = 0; i < len; i++) {
			pout->OutChar(str[i]);
			if (str[i] == '\n' && !TableEngineActive()) pout->OutFormat("<br>\n");
		}
		return len;
	}
	
	stduint start = 0;
	for (stduint i = 0; i < len; i++) {
		if (str[i] == '\n') {
			if (i > start) {
				if (fmt.U) pout->OutFormat("<u>");
				if (fmt.B) pout->OutFormat("<b>");
				if (fmt.I) pout->OutFormat("<i>");
				if (fmt.M) pout->OutFormat("<code>");
				pout->out(str + start, i - start);
				if (fmt.M) pout->OutFormat("</code>");
				if (fmt.I) pout->OutFormat("</i>");
				if (fmt.B) pout->OutFormat("</b>");
				if (fmt.U) pout->OutFormat("</u>");
			}
			if (!TableEngineActive()) pout->OutFormat("<br>\n");
			start = i + 1;
		}
	}
	if (start < len) {
		if (fmt.U) pout->OutFormat("<u>");
		if (fmt.B) pout->OutFormat("<b>");
		if (fmt.I) pout->OutFormat("<i>");
		if (fmt.M) pout->OutFormat("<code>");
		pout->out(str + start, len - start);
		if (fmt.M) pout->OutFormat("</code>");
		if (fmt.I) pout->OutFormat("</i>");
		if (fmt.B) pout->OutFormat("</b>");
		if (fmt.U) pout->OutFormat("</u>");
	}
	
	fmt_last = fmt;
	fmt_valid = true;
	return len;
}
int ProcessorTex::out(const char* str, stduint len) {
	if (!fmt.B && !fmt.I && !fmt.U && !fmt.M) {
		for (stduint i = 0; i < len; i++) {
			pout->OutChar(str[i]);
			if (str[i] == '\n' && !TableEngineActive()) pout->OutChar('\n');
		}
		return len;
	}
	
	stduint start = 0;
	for (stduint i = 0; i < len; i++) {
		if (str[i] == '\n') {
			if (i > start) {
				if (fmt.U) pout->OutFormat("\\underline{");
				if (fmt.B) pout->OutFormat("\\textbf{");
				if (fmt.I) pout->OutFormat("\\textit{");
				if (fmt.M) pout->OutFormat("\\verb|");
				pout->out(str + start, i - start);
				if (fmt.M) pout->OutFormat("|");
				if (fmt.I) pout->OutFormat("}");
				if (fmt.B) pout->OutFormat("}");
				if (fmt.U) pout->OutFormat("}");
			}
			if (!TableEngineActive()) pout->OutFormat("\n\n");
			start = i + 1;
		}
	}
	if (start < len) {
		if (fmt.U) pout->OutFormat("\\underline{");
		if (fmt.B) pout->OutFormat("\\textbf{");
		if (fmt.I) pout->OutFormat("\\textit{");
		if (fmt.M) pout->OutFormat("\\verb|");
		pout->out(str + start, len - start);
		if (fmt.M) pout->OutFormat("|");
		if (fmt.I) pout->OutFormat("}");
		if (fmt.B) pout->OutFormat("}");
		if (fmt.U) pout->OutFormat("}");
	}
	
	fmt_last = fmt;
	fmt_valid = true;
	return len;
}
int ProcessorMarkdown::out(const char* str, stduint len) { 
	if (!fmt.B && !fmt.I && !fmt.U && !fmt.M) {
		return pout->out(str, len);
	}
	
	stduint start = 0;
	for (stduint i = 0; i < len; i++) {
		if (str[i] == '\n') {
			if (i > start) {
				if (fmt.U) pout->OutFormat("<u>");
				if (fmt.B) pout->OutFormat("**");
				if (fmt.I) pout->OutFormat("*");
				if (fmt.M) pout->OutFormat("`");
				pout->out(str + start, i - start);
				if (fmt.M) pout->OutFormat("`");
				if (fmt.I) pout->OutFormat("*");
				if (fmt.B) pout->OutFormat("**");
				if (fmt.U) pout->OutFormat("</u>");
			}
			pout->OutChar('\n');
			start = i + 1;
		}
	}
	if (start < len) {
		if (fmt.U) pout->OutFormat("<u>");
		if (fmt.B) pout->OutFormat("**");
		if (fmt.I) pout->OutFormat("*");
		if (fmt.M) pout->OutFormat("`");
		pout->out(str + start, len - start);
		if (fmt.M) pout->OutFormat("`");
		if (fmt.I) pout->OutFormat("*");
		if (fmt.B) pout->OutFormat("**");
		if (fmt.U) pout->OutFormat("</u>");
	}
	
	fmt_last = fmt;
	fmt_valid = true;
	return len;
}
int ProcessorStdout::out(const char* str, stduint len) { 
	if (!fmt_valid) { // B I U
		uni::String str;
		if (!fmt.B && !fmt.I && !fmt.U) str = "0";
		else {
			if (fmt.B) str += "1;";
			if (fmt.I) str += "3;";
			if (fmt.U) str += "4;";
			str[-1] = '\0';
			str.Refresh();
		}
		pout->OutFormat("\033[%sm", str.reference());
		fmt_last = fmt;
		fmt_valid = true;
	}
	return pout->out(str, len);
}


bool ProcessorHTML::proc(uni::Nnode* nod)
{
	return process(nod, this);
}
bool ProcessorTex::proc(uni::Nnode* nod)
{
	return process(nod, this);
}
bool ProcessorMarkdown::proc(uni::Nnode* nod)
{
	return process(nod, this);
}
bool ProcessorStdout::proc(uni::Nnode* nod)
{
	return process(nod, this);
}
