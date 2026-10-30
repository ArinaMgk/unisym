#if !defined(_Linux) && !defined(_WinNT)
#define _Linux
#endif

#include "../inc/cpp/unisym"

#include "../inc/c/node.h"
#include "../inc/c/dnode.h"
#include "../inc/c/nnode.h"
#include "../inc/c/consio.h"
#include "../inc/cpp/Lango/Frontend.hpp"
#include "../inc/cpp/string"
#include "../inc/cpp/vector"

// Frontend selected for the current Magice compilation unit.
enum class MagicFrontend {
	C, // C language frontend.
};

// Resolve an explicit --lango value or a recognized source-file suffix.
bool FrontendFromName(rostr name, MagicFrontend& frontend);
bool FrontendFromPath(rostr path, MagicFrontend& frontend);

// Construct the selected source-language frontend.
uni::lango::FrontendInterface* FrontendCreate(MagicFrontend frontend);

// Print a language-specific syntax tree without extending the common interface.
bool FrontendPrintAST(MagicFrontend frontend, uni::IstreamTrait& input,
	uni::OstreamTrait& output, const uni::lango::FrontendOptions& options);

// Frontend variable table. It will grow into the declaration and scope table.
struct Variable {
	uni::NodeChain variables;
	Variable();
	void List(uni::Nchain* nchain);
	stdsint GetOffset(rostr iden);
	stduint GetTotalSize() { return 8 * variables.Count(); }
};
void DelVari(pureptr_t p);

// ---- Magice for MarkLike

enum magrk_token {
	magrkt_Text = tok__continue,

	magrkt__continue
};

struct MagrkText {
	char* addr;
	bool B;
	bool I;
	// Underline, Deleteline are functions...
};

int mark(int argc, char** argv);
