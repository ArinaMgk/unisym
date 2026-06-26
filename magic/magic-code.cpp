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
    
    rostr firstline_str = nullptr;
    rostr lastline_str = nullptr;
    if (chain->Count() > 2) firstline_str = SeekString((*chain)[2], proc);
    if (chain->Count() > 3) lastline_str = SeekString((*chain)[3], proc);
    
    int firstline = firstline_str ? atoins(firstline_str) : 1;
    int lastline = lastline_str ? atoins(lastline_str) : 0;
    
    if (!lang || !filepath) return;
    
    uni::String resolved_path = ResolveIncludePath(filepath);
    HostFile file(resolved_path.reference());
    if (!file) return;
    
    in_code_block = true;

    // Begin block
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        if (lang[0]) proc->OutFormat("<pre><code class=\"language-%s\">", lang);
        else proc->OutFormat("<pre><code>");
        break;
    case MarkProcessor::TextFormat::Tex:
        if (lang[0]) {
            if (firstline > 1) {
                proc->OutFormat("\\begin{lstlisting}[language=%s, firstnumber=%d]\n", lang, firstline);
            } else {
                proc->OutFormat("\\begin{lstlisting}[language=%s]\n", lang);
            }
        } else {
            proc->OutFormat("\\begin{verbatim}\n");
        }
        break;
    case MarkProcessor::TextFormat::Markdown:
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("```%s\n", lang);
        break;
    }
    
    // Output content
    int ch;
    int current_line = 1;
    bool in_range = (current_line >= firstline) && (lastline == 0 || current_line <= lastline);
    
    while ((ch = file.inn()) != EOF) {
        if (ch == '\r') continue; // Normalize CRLF to LF
        
        if (in_range) {
            proc->OutChar(ch);
        }
        
        if (ch == '\n') {
            current_line++;
            in_range = (current_line >= firstline) && (lastline == 0 || current_line <= lastline);
            if (lastline > 0 && current_line > lastline) {
                break; // Stop reading early if we've passed lastline
            }
        }
    }
    in_code_block = false;
    
    // End block
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("</code></pre>");
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

// Code(lang, content)
void GF_Code(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    if (chain->Count() < 2) return;
    
    rostr lang = SeekString((*chain)[0], proc);
    rostr content = SeekString((*chain)[1], proc);
    
    if (!lang || !content) return;
    
    in_code_block = true;

    // Begin block
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        if (lang[0]) proc->OutFormat("<pre><code class=\"language-%s\">", lang);
        else proc->OutFormat("<pre><code>");
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
    for (stduint i = 0; content[i]; i++) {
        if (content[i] == '\r') continue; // Normalize CRLF to LF
        proc->OutChar(content[i]);
    }
    if (content[0] != 0 && content[StrLength(content) - 1] != '\n') {
        proc->OutChar('\n');
    }
    
    in_code_block = false;
    
    // End block
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("</code></pre>");
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
