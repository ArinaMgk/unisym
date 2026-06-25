// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Compile) Mark Layer - Font Control
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "include/markproc.hpp"

// FontBegin(size, baselineskip="", family="")
void GF_FontBegin(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    if (chain->Count() < 1) return;
    rostr size = SeekString((*chain)[0], proc);
    rostr baselineskip = size;
    rostr family = nullptr;
    
    if (chain->Count() > 1) {
        rostr parsed_bl = SeekString((*chain)[1], proc);
        if (parsed_bl && parsed_bl[0]) baselineskip = parsed_bl;
    }
    if (chain->Count() > 2) {
        rostr parsed_fam = SeekString((*chain)[2], proc);
        if (parsed_fam && parsed_fam[0]) family = parsed_fam;
    }

    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("{");
        if (size && size[0]) proc->OutFormat("\\fontsize{%s}{%s}", size, baselineskip);
        if (family && family[0]) proc->OutFormat("\\fontfamily{%s}", family);
        proc->OutFormat("\\selectfont\n");
        break;
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("<div style=\"");
        if (size && size[0]) proc->OutFormat("font-size: %s;", size);
        if (family && family[0]) proc->OutFormat("font-family: %s;", family);
        proc->OutFormat("\">");
        break;
    default:
        break;
    }
}

// FontEnd()
void GF_FontEnd(uni::Dchain* chain, MarkProcessor* proc) {
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("}\n");
        break;
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("</div>");
        break;
    default:
        break;
    }
}

// FontSize(size, text)
void GF_FontSize(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    if (chain->Count() < 2) return;
    rostr size = SeekString((*chain)[0], proc);
    rostr text = SeekString((*chain)[1], proc);
    if (!size || !text) return;
    
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("{\\fontsize{%s}{%s}\\selectfont ", size, size);
        ParseAndOutputText(text, proc);
        proc->OutFormat("}");
        break;
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("<span style=\"font-size: %s;\">", size);
        ParseAndOutputText(text, proc);
        proc->OutFormat("</span>");
        break;
    default:
        ParseAndOutputText(text, proc);
        break;
    }
}

// FontFamily(family, text)
void GF_FontFamily(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    if (chain->Count() < 2) return;
    rostr family = SeekString((*chain)[0], proc);
    rostr text = SeekString((*chain)[1], proc);
    if (!family || !text) return;
    
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("{\\fontfamily{%s}\\selectfont ", family);
        ParseAndOutputText(text, proc);
        proc->OutFormat("}");
        break;
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("<span style=\"font-family: %s;\">", family);
        ParseAndOutputText(text, proc);
        proc->OutFormat("</span>");
        break;
    default:
        ParseAndOutputText(text, proc);
        break;
    }
}
