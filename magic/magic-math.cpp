// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Compile) Mark Layer - Math
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "include/markproc.hpp"

namespace {
    bool in_math_block = false;
    bool mathjax_injected = false;

    void InjectMathJax(MarkProcessor* proc) {
        if (!mathjax_injected && proc->txtfmt == MarkProcessor::TextFormat::HTML) {
            proc->OutFormat("<script id=\"MathJax-script\" async src=\"https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js\"></script>\n");
            mathjax_injected = true;
        }
    }
}

bool MathEngineActive() {
    return in_math_block;
}

// Math(text) - Inline math
void GF_Math(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    if (chain->Count() < 1) return;
    rostr text = SeekString((*chain)[0], proc);
    if (!text) return;
    
    in_math_block = true;
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("$");
        proc->out(text, StrLength(text));
        proc->OutFormat("$");
        break;
    case MarkProcessor::TextFormat::HTML:
        InjectMathJax(proc);
        proc->OutFormat("\\(");
        proc->out(text, StrLength(text));
        proc->OutFormat("\\)");
        break;
    case MarkProcessor::TextFormat::Markdown:
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("$");
        proc->out(text, StrLength(text));
        proc->OutFormat("$");
        break;
    default:
        proc->out(text, StrLength(text));
        break;
    }
    in_math_block = false;
}

// MathBlock(text) - Block math
void GF_MathBlock(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    if (chain->Count() < 1) return;
    rostr text = SeekString((*chain)[0], proc);
    if (!text) return;
    
    in_math_block = true;
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("\\[\n");
        proc->out(text, StrLength(text));
        proc->OutFormat("\n\\]\n");
        break;
    case MarkProcessor::TextFormat::HTML:
        InjectMathJax(proc);
        proc->OutFormat("\\[\n");
        proc->out(text, StrLength(text));
        proc->OutFormat("\n\\]\n");
        break;
    case MarkProcessor::TextFormat::Markdown:
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("$$\n");
        proc->out(text, StrLength(text));
        proc->OutFormat("\n$$\n");
        break;
    default:
        proc->OutFormat("\n");
        proc->out(text, StrLength(text));
        proc->OutFormat("\n");
        break;
    }
    in_math_block = false;
}
