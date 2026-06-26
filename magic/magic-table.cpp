// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Compile) Mark Layer - Table
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "include/markproc.hpp"
#include <stdio.h>

struct TableContext {
    char alignments[256];
    int current_row_span;
    bool is_in_head;
    bool is_in_table;
    int current_col_index;
    
    int rcell_cspan;
    int rcell_rspan;
    bool rcell_has_align;
    
    TableContext() : current_row_span(1), is_in_head(false), is_in_table(false), current_col_index(0), rcell_cspan(1), rcell_rspan(1), rcell_has_align(false) {
        alignments[0] = '\0';
    }
};

static TableContext tbl_ctx;

bool TableEngineActive() {
    return tbl_ctx.is_in_table;
}

static void EndPreviousRow(MarkProcessor* proc) {
    if (tbl_ctx.current_col_index > 0) {
        if (proc->txtfmt == MarkProcessor::TextFormat::Tex) {
            proc->OutFormat(" \\\\\n");
        }
        else if (proc->txtfmt == MarkProcessor::TextFormat::Markdown || proc->txtfmt == MarkProcessor::TextFormat::STDOUT) {
            proc->OutFormat("\n");
        }
    }
}

static void TransitionHeader(MarkProcessor* proc, bool next_is_head) {
    if (tbl_ctx.is_in_head && !next_is_head) {
        if (proc->txtfmt == MarkProcessor::TextFormat::Tex) {
            proc->OutFormat("\\endhead\n\\hline\n\\endfoot\n\\hline\n\\endlastfoot\n");
        }
        else if (proc->txtfmt == MarkProcessor::TextFormat::Markdown || proc->txtfmt == MarkProcessor::TextFormat::STDOUT) {
            // Output Markdown alignment line
            proc->OutFormat("|");
            for (stduint i = 0; tbl_ctx.alignments[i] != '\0'; i++) {
                char c = tbl_ctx.alignments[i];
                if (c == '|') continue;
                if (c == 'c' || c == 'C') proc->OutFormat(":---:|");
                else if (c == 'r' || c == 'R') proc->OutFormat("---:|");
                else proc->OutFormat(":---|");
            }
            proc->OutFormat("\n");
        }
    }
}

// TableBegin(alignments)
void GF_TableBegin(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    tbl_ctx.is_in_table = true;
    rostr align = nullptr;
    if (chain->Count() > 0) align = SeekString((*chain)[0], proc);
    
    if (align) {
        stduint i = 0;
        while (align[i] && i < 255) {
            tbl_ctx.alignments[i] = align[i];
            i++;
        }
        tbl_ctx.alignments[i] = '\0';
    } else {
        tbl_ctx.alignments[0] = '\0';
    }
    tbl_ctx.current_row_span = 1;
    tbl_ctx.is_in_head = false;
    tbl_ctx.current_col_index = 0;
    
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("<table>");
        break;
    case MarkProcessor::TextFormat::Tex:
        if (align) proc->OutFormat("\\begin{longtable}{|%s|}\n", align);
        else proc->OutFormat("\\begin{longtable}\n");
        proc->OutFormat("\\hline\n");
        break;
    case MarkProcessor::TextFormat::Markdown:
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("\n");
        break;
    }
}

void GF_TableEnd(uni::Dchain* chain, MarkProcessor* proc) {
    EndPreviousRow(proc);
    tbl_ctx.is_in_table = false;
    TransitionHeader(proc, false); // In case it was all headers? Unlikely, but safe.
    
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        proc->OutFormat("</table>");
        break;
    case MarkProcessor::TextFormat::Tex:
        proc->OutFormat("\\hline\n\\end{longtable}\n");
        break;
    case MarkProcessor::TextFormat::Markdown:
    case MarkProcessor::TextFormat::STDOUT:
        proc->OutFormat("\n");
        break;
    }
    tbl_ctx.current_col_index = 0;
}

// Row(rowspan=1)
void GF_Row(uni::Dchain* chain, MarkProcessor* proc) {
    EndPreviousRow(proc);
    TransitionHeader(proc, false);
    
    int rspan = 1;
    if (chain->Count() > 0) {
        rostr rs = SeekString((*chain)[0], proc);
        if (rs) rspan = atoins(rs);
    }
    
    tbl_ctx.current_row_span = rspan > 0 ? rspan : 1;
    tbl_ctx.is_in_head = false;
    tbl_ctx.current_col_index = 0;
    
    if (proc->txtfmt == MarkProcessor::TextFormat::HTML) {
        proc->OutFormat("<tr>");
    } else if (proc->txtfmt == MarkProcessor::TextFormat::Markdown || proc->txtfmt == MarkProcessor::TextFormat::STDOUT) {
        proc->OutFormat("| ");
    }
}

// RowHead(rowspan=1)
void GF_RowHead(uni::Dchain* chain, MarkProcessor* proc) {
    EndPreviousRow(proc);
    TransitionHeader(proc, true);
    
    int rspan = 1;
    if (chain->Count() > 0) {
        rostr rs = SeekString((*chain)[0], proc);
        if (rs) rspan = atoins(rs);
    }
    
    tbl_ctx.current_row_span = rspan > 0 ? rspan : 1;
    tbl_ctx.is_in_head = true;
    tbl_ctx.current_col_index = 0;
    
    if (proc->txtfmt == MarkProcessor::TextFormat::HTML) {
        proc->OutFormat("<tr class=\"head\">");
    } else if (proc->txtfmt == MarkProcessor::TextFormat::Markdown || proc->txtfmt == MarkProcessor::TextFormat::STDOUT) {
        proc->OutFormat("| ");
    }
}

void ParseAndOutputText(rostr text, MarkProcessor* proc) {
    if (!text) return;
    char* p = (char*)text;
    char ch;
    stduint chunk_start = 0;
    while ((ch = *p++)) {
        if (ch == '^') {
            stduint len = (p - 1) - (text + chunk_start);
            if (len > 0) EscapeAndOutputText(text + chunk_start, len, proc);
            
            char chh = *p++;
            if (!chh) break;
            if (chh == 'B') { proc->fmt.B = true; proc->fmt_valid = false; }
            else if (chh == 'b') { proc->fmt.B = false; proc->fmt_valid = false; }
            else if (chh == 'I') { proc->fmt.I = true; proc->fmt_valid = false; }
            else if (chh == 'i') { proc->fmt.I = false; proc->fmt_valid = false; }
            else if (chh == 'U') { proc->fmt.U = true; proc->fmt_valid = false; }
            else if (chh == 'u') { proc->fmt.U = false; proc->fmt_valid = false; }
            else if (chh == 'M') { proc->fmt.M = true; proc->fmt_valid = false; }
            else if (chh == 'm') { proc->fmt.M = false; proc->fmt_valid = false; }
            else if (chh == 'T') { proc->fmt.T = true; proc->fmt_valid = false; }
            else if (chh == 't') { proc->fmt.T = false; proc->fmt_valid = false; }
            else if (chh == '^') { 
                proc->fmt.B = proc->fmt.I = proc->fmt.U = proc->fmt.M = proc->fmt.T = false; 
                proc->fmt_valid = false; 
            }
            
            chunk_start = p - text;
        }
        else if (ch == '\n') {
            stduint len = (p - 1) - (text + chunk_start);
            if (len > 0) EscapeAndOutputText(text + chunk_start, len, proc);
            
            if (proc->txtfmt == MarkProcessor::TextFormat::Tex) {
                proc->OutFormat("\\\\");
            } else if (proc->txtfmt == MarkProcessor::TextFormat::HTML) {
                proc->OutFormat("<br>\n");
            } else {
                proc->OutFormat(" ");
            }
            chunk_start = p - text;
        }
    }
    stduint len = (p - 1) - (text + chunk_start);
    if (len > 0) EscapeAndOutputText(text + chunk_start, len, proc);
}

// Rcell(text, colspan=1, align_override="")
void GF_Rcell(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    if (chain->Count() < 1) return;
    
    rostr text = SeekString((*chain)[0], proc);
    if (!text) text = "";
    
    int cspan = 1;
    if (chain->Count() > 1) {
        rostr cs = SeekString((*chain)[1], proc);
        if (cs) cspan = atoins(cs);
    }
    if (cspan < 1) cspan = 1;
    
    rostr align = nullptr;
    if (chain->Count() > 2) {
        align = SeekString((*chain)[2], proc);
        if (align && align[0] == '\0') align = nullptr;
    }
    
    int rspan = tbl_ctx.current_row_span;
    if (chain->Count() > 3) {
        rostr rs = SeekString((*chain)[3], proc);
        if (rs) {
            int rs_val = atoins(rs);
            if (rs_val > 0) rspan = rs_val;
        }
    }
    
    bool ishead = tbl_ctx.is_in_head;
    
    bool has_newline = false;
    for (stduint i = 0; text[i]; i++) {
        if (text[i] == '\n') has_newline = true;
    }
    
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        {
            const char* tag = ishead ? "th" : "td";
            proc->OutFormat("<%s", tag);
            if (rspan > 1) proc->OutFormat(" rowspan=\"%d\"", rspan);
            if (cspan > 1) proc->OutFormat(" colspan=\"%d\"", cspan);
            if (align) proc->OutFormat(" style=\"text-align:%s\"", align);
            proc->OutFormat(">");
            ParseAndOutputText(text, proc);
            proc->OutFormat("</%s>\n", tag);
        }
        break;
    case MarkProcessor::TextFormat::Tex:
        {
            if (tbl_ctx.current_col_index > 0) proc->OutFormat(" & ");
            
            bool use_multicolumn = (cspan > 1 || align);
            if (use_multicolumn) {
                const char* alg = align ? align : "c";
                if (rspan > 1) {
                    proc->OutFormat("\\multicolumn{%d}{%s}{\\multirow{%d}{*}{", cspan, alg, rspan);
                } else {
                    proc->OutFormat("\\multicolumn{%d}{%s}{", cspan, alg);
                }
            } else if (rspan > 1) {
                proc->OutFormat("\\multirow{%d}{*}{", rspan);
            }
            if (has_newline) proc->OutFormat("\\makecell{");
            
            ParseAndOutputText(text, proc);
            
            if (has_newline) proc->OutFormat("}");
            if (use_multicolumn) {
                if (rspan > 1) proc->OutFormat("}}");
                else proc->OutFormat("}");
            } else if (rspan > 1) {
                proc->OutFormat("}");
            }
        }
        break;
    default:
        ParseAndOutputText(text, proc);
        proc->OutFormat(" | ");
        break;
    }
    
    tbl_ctx.current_col_index += cspan;
}

// RcellSkip(colspan = 1)
void GF_RcellSkip(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    int cspan = 1;
    if (chain->Count() > 0) {
        rostr cs = SeekString((*chain)[0], proc);
        if (cs) cspan = atoins(cs);
    }
    if (cspan < 1) cspan = 1;
    
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        // HTML automatically skips spanned cells, so we output nothing!
        break;
    case MarkProcessor::TextFormat::Tex:
        for (int i = 0; i < cspan; i++) {
            if (tbl_ctx.current_col_index > 0 || i > 0) proc->OutFormat(" & ");
        }
        break;
    default:
        // Markdown
        for (int i = 0; i < cspan; i++) {
            proc->OutFormat(" | ");
        }
        break;
    }
    
    tbl_ctx.current_col_index += cspan;
}

// LineH([start_col], [end_col])
void GF_LineH(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    EndPreviousRow(proc);
    
    int start = 0, end = 0;
    if (chain->Count() > 0) {
        rostr s = SeekString((*chain)[0], proc);
        if (s && s[0] != '\0') start = atoins(s);
    }
    if (chain->Count() > 1) {
        rostr e = SeekString((*chain)[1], proc);
        if (e && e[0] != '\0') end = atoins(e);
    }
    
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        break;
    case MarkProcessor::TextFormat::Tex:
        if (start > 0 && end >= start) {
            proc->OutFormat("\\cline{%d-%d}\n", start, end);
        } else if (start > 0) {
            proc->OutFormat("\\cline{%d-%d}\n", start, start);
        } else {
            proc->OutFormat("\\hline\n");
        }
        break;
    default:
        break;
    }
    tbl_ctx.current_col_index = 0;
}

// TableCSV(filepath, has_header)
void GF_TableCSV(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    if (chain->Count() < 1) return;
    rostr filepath = SeekString((*chain)[0], proc);
    bool has_header = false;
    if (chain->Count() > 1) {
        rostr hh = SeekString((*chain)[1], proc);
        if (hh && hh[0] != '0' && hh[0] != '\0') has_header = true;
    }
    
    if (!filepath) return;
    
    uni::String resolved_path = ResolveIncludePath(filepath);
    
    FILE* fp = fopen(resolved_path.reference(), "r");
    if (!fp) return;
    
    char line[4096];
    bool is_first_line = true;
    
    while (fgets(line, sizeof(line), fp)) {
        stduint len = StrLength(line);
        if (len > 0 && line[len-1] == '\n') line[--len] = '\0';
        if (len > 0 && line[len-1] == '\r') line[--len] = '\0';
        
        if (line[0] == '\0') continue;
        
        uni::Dchain dummy_chain;
        if (is_first_line) {
            if (has_header) GF_RowHead(&dummy_chain, proc);
            else GF_Row(&dummy_chain, proc);
        } else {
            GF_LineH(&dummy_chain, proc);
            GF_Row(&dummy_chain, proc);
        }
        is_first_line = false;
        
        char* start = line;
        bool in_quotes = false;
        for (char* p = line; *p; p++) {
            if (*p == '"') {
                in_quotes = !in_quotes;
            } else if (*p == ',' && !in_quotes) {
                *p = '\0';
                uni::Dchain cell_chain;
                uni::Dnode* dn = cell_chain.AppendHeapstr(start);
                dn->type = tok_string;
                GF_Rcell(&cell_chain, proc);
                start = p + 1;
            }
        }
        uni::Dchain cell_chain;
        uni::Dnode* dn = cell_chain.AppendHeapstr(start);
        dn->type = tok_string;
        GF_Rcell(&cell_chain, proc);
    }
    fclose(fp);
}

void GF_RcellBegin(uni::Dchain* chain, MarkProcessor* proc) {
    using namespace uni;
    int cspan = 1;
    if (chain->Count() > 0) {
        rostr cs = SeekString((*chain)[0], proc);
        if (cs) cspan = atoins(cs);
    }
    if (cspan < 1) cspan = 1;
    
    rostr align = nullptr;
    if (chain->Count() > 1) {
        align = SeekString((*chain)[1], proc);
        if (align && align[0] == '\0') align = nullptr;
    }
    
    int rspan = tbl_ctx.current_row_span;
    if (chain->Count() > 2) {
        rostr rs = SeekString((*chain)[2], proc);
        if (rs) {
            int rs_val = atoins(rs);
            if (rs_val > 0) rspan = rs_val;
        }
    }
    
    tbl_ctx.rcell_cspan = cspan;
    tbl_ctx.rcell_rspan = rspan;
    tbl_ctx.rcell_has_align = (align != nullptr);
    
    bool ishead = tbl_ctx.is_in_head;
    
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        {
            const char* tag = ishead ? "th" : "td";
            proc->OutFormat("<%s", tag);
            if (rspan > 1) proc->OutFormat(" rowspan=\"%d\"", rspan);
            if (cspan > 1) proc->OutFormat(" colspan=\"%d\"", cspan);
            if (align) proc->OutFormat(" style=\"text-align:%s\"", align);
            proc->OutFormat(">");
        }
        break;
    case MarkProcessor::TextFormat::Tex:
        {
            if (tbl_ctx.current_col_index > 0) proc->OutFormat(" & ");
            
            bool use_multicolumn = (cspan > 1 || align);
            if (use_multicolumn) {
                const char* alg = align ? align : "c";
                if (rspan > 1) {
                    proc->OutFormat("\\multicolumn{%d}{%s}{\\multirow{%d}{*}{", cspan, alg, rspan);
                } else {
                    proc->OutFormat("\\multicolumn{%d}{%s}{", cspan, alg);
                }
            } else if (rspan > 1) {
                proc->OutFormat("\\multirow{%d}{*}{", rspan);
            }
            proc->OutFormat("\\makecell{");
        }
        break;
    default:
        break;
    }
    
    tbl_ctx.current_col_index += cspan;
}

void GF_RcellEnd(uni::Dchain* chain, MarkProcessor* proc) {
    bool ishead = tbl_ctx.is_in_head;
    switch (proc->txtfmt) {
    case MarkProcessor::TextFormat::HTML:
        {
            const char* tag = ishead ? "th" : "td";
            proc->OutFormat("</%s>\n", tag);
        }
        break;
    case MarkProcessor::TextFormat::Tex:
        {
            bool use_multicolumn = (tbl_ctx.rcell_cspan > 1 || tbl_ctx.rcell_has_align);
            proc->OutFormat("}");
            if (use_multicolumn) {
                if (tbl_ctx.rcell_rspan > 1) proc->OutFormat("}}");
                else proc->OutFormat("}");
            } else if (tbl_ctx.rcell_rspan > 1) {
                proc->OutFormat("}");
            }
        }
        break;
    default:
        proc->OutFormat(" | ");
        break;
    }
}
