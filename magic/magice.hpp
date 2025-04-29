#if !defined(_Linux) && !defined(_WinNT)
#define _Linux
#endif

#include "../inc/cpp/unisym"

#include "../inc/c/node.h"
#include "../inc/c/dnode.h"
#include "../inc/c/nnode.h"
#include "../inc/c/consio.h"
#include "../inc/cpp/string"

// ---- Magice based on C++

struct Operators {
	// make operator list
	static void List(uni::Chain& nc);
};

struct Variable {
	uni::NodeChain variables;
	Variable();
	void List(uni::Nchain* nchain);
	stdsint GetOffset(rostr iden);
	stduint GetTotalSize() { return 8 * variables.Count(); }
};
void DelVari(pureptr_t p);

enum magic_token {
	magict_function = tok__continue,
	magict_sta_declaration,
	magict_sta_return,

	magict__continue
};


// ---- Magice for MarkLike

enum magrk_token {
	magrkt_TODO = magict__continue,

	magrkt__continue
};

struct MagrkText {
	char* addr;
	bool B;
	bool I;
	// Underline, Deleteline are functions...
};

void mark(int argc, char** argv);
