#if !defined(_Linux) && !defined(_WinNT)
#define _Linux
#endif

#include "../inc/cpp/unisym"

#include "../inc/c/node.h"
#include "../inc/c/consio.h"
#include "../inc/cpp/string"

struct Operators {
	// make operator list
	static void List(uni::Chain& nc);
};

