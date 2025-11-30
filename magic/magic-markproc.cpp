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

int ProcessorHTML::out(const char* str, stduint len) { return pout->out(str, len); }
int ProcessorTex::out(const char* str, stduint len) { return pout->out(str, len); }
int ProcessorMarkdown::out(const char* str, stduint len) { return pout->out(str, len); }
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
