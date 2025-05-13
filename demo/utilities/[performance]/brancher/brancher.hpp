#define _STYLE_RUST
#ifndef _Linux
#define _Linux
#endif

#include <c/file.h>
#include <c/consio.h>
#include <cpp/nodes>



struct Linksym {
	uint64 offs;
	char* name;
};
struct Linksyms {
	static bool StructSymbols(uni::Dchain& dc, uni::HostFile& symfile);

	static stduint Index(uni::Dchain& dc, const char* fname, stduint bbnumber);
};

// nm
void BrancherFree_Linksym(pureptr_t inp);

