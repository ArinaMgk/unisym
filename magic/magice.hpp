#if !defined(_Linux) && !defined(_WinNT)
#define _Linux
#endif

#include "../inc/cpp/unisym"

#include "../inc/c/node.h"
#include "../inc/c/dnode.h"
#include "../inc/c/nnode.h"
#include "../inc/c/consio.h"
#include "../inc/cpp/string"
#include "../inc/cpp/vector"

// ---- Magice based on C++

struct Operators {
	// make operator list
	static void List(uni::Chain& nc);
};

// Frontend selected for the current Magice compilation unit.
enum class MagicFrontend {
	C, // C language frontend.
};

// Source text retained for one frontend statement when -g is enabled.
struct MagicDebugSource {
	const uni::Nnode* node = nullptr;
	uni::String text;
};

// Optional source mapping passed from the frontend to Clover IR lowering.
struct MagicDebugInfo {
	uni::Vector<MagicDebugSource> sources;

	void Add(const uni::Nnode* node, const uni::String& text);
	const uni::String* Find(const uni::Nnode* node) const;
};

// Run the selected language frontend over the generic nested parser.
bool FrontendAnalyze(MagicFrontend frontend, uni::NestedParseUnit& parser,
	MagicDebugInfo* debug_info = nullptr);

// C-specific AST preparation and validation implemented by magic-statement.cpp.
bool FrontendCPrepare(uni::Nchain& syntax_tree, MagicDebugInfo* debug_info = nullptr);
bool FrontendCValidate(uni::Nchain& syntax_tree);

// Frontend variable table. It will grow into the declaration and scope table.
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
	// C if statement. Its child nodes contain the condition expression.
	magict_sta_if,
	// C else marker paired with the nearest unmatched if statement.
	magict_sta_else,
	// C empty statement produced by a semicolon with no expression.
	magict_sta_empty,
	// C return statement or return marker followed by its expression.
	magict_sta_return,

	magict__continue
};


// ---- Magice for MarkLike

enum magrk_token {
	magrkt_Text = magict__continue,

	magrkt__continue
};

struct MagrkText {
	char* addr;
	bool B;
	bool I;
	// Underline, Deleteline are functions...
};

int mark(int argc, char** argv);
