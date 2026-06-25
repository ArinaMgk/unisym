// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Compile) Mark Layer - List
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "include/markproc.hpp"

struct ListContext {
    bool is_in_list;
    char list_type[32]; // e.g., "itemize", "enumerate"
    
    ListContext() : is_in_list(false) {
        list_type[0] = '\0';
    }
};

static ListContext lst_ctx;

void GF_ListBegin(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    lst_ctx.is_in_list = true;
    rostr type = nullptr;
    if (chain->Count() > 0) type = SeekString((*chain)[0], proc);
    
    if (type) StrCopy(lst_ctx.list_type, type);
    else StrCopy(lst_ctx.list_type, "itemize");
    
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        {
            const char* tag = (!StrCompare(lst_ctx.list_type, "enumerate")) ? "ol" : "ul";
            proc->OutFormat("<%s>", tag);
        }break;
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("\\begin{%s}\n", lst_ctx.list_type);
        break;
    case MarkProcessor::TextFormat::Markdown:
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("\n");
        break;
    }
}

void GF_ListEnd(uni::Dchain* chain, MarkProcessor* proc) {
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        {
            const char* tag = (!StrCompare(lst_ctx.list_type, "enumerate")) ? "ol" : "ul";
            proc->OutFormat("</%s>", tag);
        }break;
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("\\end{%s}\n", lst_ctx.list_type);
        break;
    case MarkProcessor::TextFormat::Markdown:
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("\n");
        break;
    }
    lst_ctx.is_in_list = false;
    lst_ctx.list_type[0] = '\0';
}

void GF_ListItem(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    rostr prefix = nullptr;
    rostr main_text = nullptr;
    
    if (chain->Count() > 0) prefix = SeekString((*chain)[0], proc);
    if (chain->Count() > 1) main_text = SeekString((*chain)[1], proc);
    
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("<li>");
        if (prefix && prefix[0] != '\0') proc->OutFormat("<code>%s</code> : ", prefix);
        if (main_text) ParseAndOutputText(main_text, proc);
        proc->OutFormat("</li>");
        break;
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("\\item");
        if (prefix && prefix[0] != '\0') proc->OutFormat("\\verb`%s` : ", prefix);
        else proc->OutFormat(" ");
        if (main_text) ParseAndOutputText(main_text, proc);
        proc->OutFormat("\n");
        break;
    case MarkProcessor::TextFormat::Markdown:
    case MarkProcessor::TextFormat::STDOUT:
        if (!StrCompare(lst_ctx.list_type, "enumerate")) proc->OutFormat("1. ");
        else proc->OutFormat("- ");
        
        if (prefix && prefix[0] != '\0') proc->OutFormat("`%s` : ", prefix);
        if (main_text) ParseAndOutputText(main_text, proc);
        proc->OutFormat("\n");
        break;
    }
}
