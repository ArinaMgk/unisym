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

bool Linksyms::StructSymbols(uni::Dchain& dc, HostFile& symfile) {
	//{TODO} specified symbol names
	if (!bool(symfile)) return false;
	char* buffer = (char*)malc(2048);
	while (symfile.FetchLine(buffer, _TEMP 2048)) {
		bool skip = false;
		if (!ascii_isxdigit(buffer[0])) continue;
		const char* star_diver = StrIndexChar(buffer, '-');// or *
		if (!star_diver) continue;
		for0a(i, black_list) {
			if (StrIndexString(buffer, black_list[i])) {
				skip = true;
				break;
			}
		}
		if (skip) continue;
		const char* nam_beg = StrIndexString(buffer, " t ");
		if (!nam_beg) continue;
		nam_beg += 3;

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
