// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Compile) Mark Layer - Code Block
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "include/markproc.hpp"
#include "../inc/c/file.h"

namespace {
    bool in_code_block = false;
}

bool CodeEngineActive() {
    return in_code_block;
}

// CodeInclude(lang, filepath)
void GF_CodeInclude(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    if (chain->Count() < 2) return;
    
    rostr lang = SeekString((*chain)[0], proc);
    rostr filepath = SeekString((*chain)[1], proc);
    if (!lang || !filepath) return;
    
    uni::String resolved_path = ResolveIncludePath(filepath);
    HostFile file(resolved_path.reference());
    if (!file) return;
    
    // Begin block
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        if (lang[0]) proc->OutFormat("<pre><code class=\"language-%s\">\n", lang);
        else proc->OutFormat("<pre><code>\n");
        break;
    case MarkProcessor::TextFormat::Tex:
        if (lang[0]) proc->OutFormat("\\begin{lstlisting}[language=%s]\n", lang);
        else proc->OutFormat("\\begin{verbatim}\n");
        break;
    case MarkProcessor::TextFormat::Markdown:
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("```%s\n", lang);
        break;
    }
    
    // Output content
    in_code_block = true;
    int ch;
    while ((ch = file.inn()) != EOF) {
        if (ch == '\r') continue; // Normalize CRLF to LF
        proc->OutChar(ch);
    }
    in_code_block = false;
    
    // End block
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("</code></pre>\n");
        break;
    case MarkProcessor::TextFormat::Tex:
        if (lang[0]) proc->OutFormat("\\end{lstlisting}\n");
        else proc->OutFormat("\\end{verbatim}\n");
        break;
    case MarkProcessor::TextFormat::Markdown:
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("\n```\n\n");
        break;
    }
}
