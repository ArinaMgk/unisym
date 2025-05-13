// ASCII CPP-ISO2x TAB4 CRLF
// Docutitle: Brancher Main Program
// Codifiers: @dosconio: 20250211- 
// Attribute: Host(Linux)
// Copyright: Apache License 2.0

#include "brancher.hpp"
#include <c/consio.h>

use crate uni;

static rostr black_list[] = {
	"__cxx_global_var_init*",
	"_GLOBAL__sub_I_c.cpp*"
};

extern Dchain* want_trace;
bool Linksyms::StructSymbols(uni::Dchain& dc, HostFile& symfile) {
	//{TODO} specified symbol names
	if (!bool(symfile)) return false;
	char* buffer = (char*)malc(2048);
	while (symfile.FetchLine(buffer, _TEMP 2048)) {
		bool skip = false;
		if (!ascii_isxdigit(buffer[0])) continue;
		const char* star_diver = StrIndexChar(buffer, '-');// or *
		if (!star_diver) continue;

		// match name: C and GNU C++( _Z3fffii )
		if (want_trace && want_trace->Count());
		else for0a(i, black_list) {
			if (StrIndexString(buffer, black_list[i])) {
				skip = true;
				break;
			}
		}

		if (skip) continue;
		const char* nam_beg = StrIndexString(buffer, " t ");
		if (!nam_beg) continue;
		nam_beg += 3;

		if (want_trace && want_trace->Count()) {
			skip = true;
			for (auto var = want_trace->Root(); var; var = var->next) {
				rostr tmptmp = nam_beg;
				stduint len = StrIndexChar(nam_beg, '*') - nam_beg;
				if (!(var->addr[0] == '_' && var->addr[1] == 'Z') && tmptmp[0] == '_' && tmptmp[1] == 'Z') {
					tmptmp += 2;
					len = atoins(tmptmp);
					while (ascii_isdigit(*tmptmp)) tmptmp++;
					// ploginfo("> %s %s %u", tmptmp, var->addr, len);
				}// GNU C++

				if (!StrCompareN(tmptmp, var->addr, len) && (var->addr[len] == '\0')) {
					skip = false;
					break;
				}
			}
			if (skip) continue;
		}

		Linksym* sym = malcof(Linksym);
		sym->offs = atohex(buffer);
		sym->name = StrHeapN(nam_beg, star_diver - nam_beg);
		dc.Append((pureptr_t)sym);

		//Console.OutFormat("\n%s%[64H] %s\n", buffer, sym->offs, sym->name);
	}
	mfree(buffer);
	return true;
}

stduint Linksyms::Index(uni::Dchain& dc, const char* fname, stduint bbnumber) {
	String want = String::newFormat("%s*%[u]", fname, bbnumber);
	for (auto var = dc.Root(); var; var = var->next) {
		Linksym* linksym = (Linksym*)var->offs;
		if (!StrCompare(want.reference(), linksym->name))
			return linksym->offs;
	}
	return nil;
}

void BrancherFree_Linksym(pureptr_t inp) {
	Letvar(nod, Dnode*, inp);
	Letvar(sym, Linksym*, nod->offs);
	memf(sym->name);
	memf(nod->offs);
}
