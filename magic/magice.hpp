#if !defined(_Linux) && !defined(_WinNT)
#define _Linux
#endif

#include "../inc/cpp/unisym"

#include "../inc/c/node.h"
#include "../inc/c/consio.h"
#include "../inc/cpp/string"

// ---- Magice based on C++

struct Operators {
	// make operator list
	static void List(uni::Chain& nc);
};

enum magic_token {
	magict_TODO = tok__continue,

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
