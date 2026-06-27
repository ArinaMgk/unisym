// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Compile) Mark Layer - Bibliography
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "include/markproc.hpp"

// BibBegin(widest_label="99")
void GF_BibBegin(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    rostr label = "99";
    if (chain->Count() > 0) {
        rostr parsed_label = SeekString((*chain)[0], proc);
        if (parsed_label && parsed_label[0]) label = parsed_label;
    }

    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("<div class=\"bibliography\">");
        break;
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("\\begin{thebibliography}{%s}\n", label);
        break;
    case MarkProcessor::TextFormat::Markdown:
        proc->OutFormat("---\n**References**\n\n");
        break;
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("\n[ References ]\n");
        break;
    }
}

// BibEnd()
void GF_BibEnd(uni::Dchain* chain, MarkProcessor* proc) {
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("</div>");
        break;
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("\\end{thebibliography}\n");
        break;
    case MarkProcessor::TextFormat::Markdown:
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("\n");
        break;
    }
}

// Cite(id)
void GF_Cite(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    if (chain->Count() < 1) return;
    rostr id = SeekString((*chain)[0], proc);
    if (!id) return;

    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("<a href=\"#ref-%s\">[%s]</a>", id, id);
        break;
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("\\cite{%s}", id);
        break;
    case MarkProcessor::TextFormat::Markdown:
        proc->OutFormat("[[%s]](#ref-%s)", id, id);
        break;
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("[%s]", id);
        break;
    }
}

// Refer(id)
void GF_Refer(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    if (chain->Count() < 1) return;
    rostr id = SeekString((*chain)[0], proc);
    if (!id) return;

    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("<a href=\"#%s\">%s</a>", id, id);
        break;
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("\\refer{%s}", id);
        break;
    case MarkProcessor::TextFormat::Markdown:
        proc->OutFormat("[%s](#%s)", id, id);
        break;
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("[%s]", id);
        break;
    }
}

// BibItem(id, text)
void GF_BibItem(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    if (chain->Count() < 2) return;
    rostr id = SeekString((*chain)[0], proc);
    rostr text = SeekString((*chain)[1], proc);
    if (!id || !text) return;

    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("<p id=\"ref-%s\">[%s] ", id, id);
        ParseAndOutputText(text, proc);
        proc->OutFormat("</p>");
        break;
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("\\bibitem{%s} ", id);
        ParseAndOutputText(text, proc);
        proc->OutFormat("\n");
        break;
    case MarkProcessor::TextFormat::Markdown:
        proc->OutFormat("<a id=\"ref-%s\"></a>[%s] ", id, id);
        ParseAndOutputText(text, proc);
        proc->OutFormat("\n\n");
        break;
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("[%s] ", id);
        ParseAndOutputText(text, proc);
        proc->OutFormat("\n");
        break;
    }
}
