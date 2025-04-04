// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Data) Token Node Parse Instructions and Directives
// Codifiers: @ArinaMgk 
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../../inc/cpp/parse.hpp"
#include <new>

static toktype getctype(char chr)// Excluding Number
{
	if (ascii_isalnum(chr))
		return tok_identy;
	else if (ascii_isspace(chr))
		return tok_spaces;
	else if (ascii_ispunct(chr))
		return tok_symbol;// '_' may be of identifier
	return tok_others;
}

// It may be better to use Regular Expression
const char static EscSeq[] =
{
	'n','\n','r','\r','a','\a','b','\b','f','\f','t','\t','v','\v'
};// other equal to literal char except the "\x"[0~2] "\"[0~2]

namespace uni
{
	LinearParser::LinearParser(IstreamTrait& lparser) {
		src = &lparser;
		buf = new String(String::Charset::Memory);
		tobuf = new String(String::Charset::Memory);
	}
	LinearParser::LinearParser(IstreamTrait& lparser, char* addr, stduint buflen) {
		src = &lparser;
		buf = NULL;// or static_lnbuf
		new (static_lnbuf) String(addr, buflen);
		_TODO tobuf = new String(String::Charset::Memory);
	}
	LinearParser::LinearParser(IstreamTrait& lparser, String& buf) {
		src = &lparser;
		this->buf = &buf;
		_TODO tobuf = new String(String::Charset::Memory);
	}

	static Tnode* TokenAppend(Dchain& chn,
		rostr content,
		size_t contlen,
		size_t ttype,
		size_t row,
		size_t col
	)
	{
		if (!contlen && ttype != tok_string) return 0;
		Letvar(tn, Tnode*, chn.Append(StrHeapN(content, contlen), false));
		tn->type = ttype;
		tn->row = row;
		tn->col = col;
		return tn;
	}

	int LinearParser::getChar() {
		int ch = todobuf->inn();
		if (ch == EOF) ch = askChar();
		pos.x += 1;
		return ch;
	}

	#define apd() do { if (!(method_omit_spaces && CrtTType == tok_spaces)) TokenAppend(dc, linebuf->reference(), linebuf->getCharCount(), CrtTType, crtline_ento, crtcol_ento); linebuf->Clear(); } while (0)

	stduint LineParse_NumChk_Default(LinearParser& lp) {
		size_t res = 0;
		int c, lastc;
		int last_zo_num = 0;
		struct OnceOccur {
			unsigned bodx : 1;
			unsigned e : 1;
			unsigned dot : 1;
			unsigned sign : 1;
		} OnceO = { 0 };
		while ((c = lp.getChar()) != EOF) {
			if (StrIndexChar("bodxij", c))// 'i' and 'j' for imagine, appended Haruno RFC05
				if (OnceO.bodx)
					break;
				else
				{
					last_zo_num = 0; OnceO.bodx = 1;
				}
			else if (c == 'e')
				if (OnceO.e || !last_zo_num)
					break;
				else
				{
					last_zo_num = 0; OnceO.e = 1;
				}
			else if (c == '.')
				if (OnceO.dot)
					break;
				else
				{
					last_zo_num = 0; OnceO.dot = 1;
				}
			else if (c == '+' || c == '-')
				if (OnceO.sign || !OnceO.e || lastc != 'e')
					break;
				else
				{
					last_zo_num = 0; OnceO.sign = 1;
				}
			else if (c >= '0' && c <= '9') last_zo_num = 1;
			else break;
			res++;
			lp.setChar(c);
			lastc = c;
		}
		if (res > 1 && (lastc == '+' || lastc == '-' || lastc == '.'))
		{
			lp.backChar(lastc);
			res--;
		}
		if (c != EOF) lp.backChar(c);
		return res;
	}

	bool LineParse_Comment_Sharp(LinearParser& lp, bool just_chk) {
		char c = lp.getChar();
		bool ret = c == '#';
		if (just_chk) return ret;
		while ((c = lp.getChar()) != EOF &&
		c != '\n' && c != '\r')
		{
			lp.setChar(c);
		}
		if (c == EOF) return ret;
		lp.backChar(c);
		return ret;
	}

	// return true for continue, false for break
	bool LinearParser::handler_escape_sequence() {
		char c;
		if ((c = getChar()) == EOF) return false;
		if (ascii_isdigit(c))
		{
			// octal: nest 0~2 3 numbers at most
			int nest1 = getChar();
			if (nest1 == EOF) return false;
			if (ascii_isdigit(nest1))
			{
				int nest2 = getChar();
				if (nest2 == EOF) return false;
				if (ascii_isdigit(nest2)) {
					// \123
					setChar((unsigned char)(nest2 - '0' + (nest1 - '0') * 8 + (c - '0') * 8 * 8));
				}
				else {
					// \12;
					setChar((nest1 - '0') + (c - '0'));
					backChar(nest2);
				}
			}
			else {
				// \1;
				setChar(c - '0');
				backChar(nest1);
			}
		}
		else if (c == 'x')// nest 0~3 and use isxdigit()
		{
			c = getChar();
			if (c == EOF) return false;
			if (ascii_isxdigit(c))
			{
				int nest1 = getChar();
				if (nest1 == EOF) return false;
				if (ascii_isxdigit(nest1))
				{
					// \x12;
					nest1 = (ascii_isdigit(nest1)) ? (nest1 - '0') : (ascii_isupper(nest1)) ? (nest1 - 'A' + 10) : (nest1 - 'a' + 10);
					c = (ascii_isdigit(c)) ? (c - '0') : (ascii_isupper(c)) ? (c - 'A' + 10) : (c - 'a' + 10);
					setChar(c * 16 + nest1);
				}
				else {
					// \x1;
					c = (ascii_isdigit(c)) ? (c - '0') : (ascii_isupper(c)) ? (c - 'A' + 10) : (c - 'a' + 10);
					setChar(c);
					backChar(nest1);
				}
			}
			else {
				// \x; = \0;
				setChar(0);
				backChar(c);
			}
		}
		else {
			// escape char and other
			if (c == EOF) return false;
			size_t listlen = sizeof(EscSeq) / 2;
			for0 (i, listlen) {
				if (EscSeq[i << 1] == c)
				{
					setChar(EscSeq[(i << 1) + 1]);
					listlen = 0;
					return false;
				}
			}
			if (!listlen) return true;
			setChar(c);
		}
		return true;
	}

	Dchain& LinearParser::Parse(Dchain& dc) {
		toktype CrtTType = tok_any;
		stduint crtline_ento = pos.x, crtcol_ento = pos.y;
		int YoString = 0;
		char strtok;
		int c;

		this->linebuf = buf ? buf : (String*)&static_lnbuf;
		String& buffer = *linebuf;
		buffer.charset = String::Charset::Memory;
		buffer.inn_direction = true;

		this->todobuf = tobuf ? tobuf : (String*)&static_tobuf;
		String& tbuf = *todobuf;
		tbuf.charset = String::Charset::Memory;
		tbuf.inn_direction = true;

		dc.func_free = DnodeHeapFreeSimple;
		dc.setExtnField(sizeof(TnodeField));
		if (GHT) TokenAppend(dc, "GHTANIRA", 1, tok_any, 0, 0);// Traditional Header Guard
		while ((c = getChar()) != EOF)
		{
			Letvar(dbg_chr, char, c);
			if (YoString)
			{
				if (strtok == c) {
					apd();// including terminated-symbol
					//pos.x += 2;
					crtline_ento = pos.y;
					crtcol_ento = pos.x - 1;
					YoString = 0;
					CrtTType = tok_any;
				}
				else if (method_string_escape_sequence && c == '\\') {
					if (handler_escape_sequence())
						continue; else break;
				}
				else setChar(c);
			}
			else if (c == '\n' || c == '\r')
			{
				// CRLF LFCR CR LF
				int cc = getChar();
				if (cc == '\n' || cc == '\r')
					if (c == cc)
					{
						backChar(cc);
					}
					else;
				else if (cc != EOF) backChar(cc);
				else break;
				if (linebuf->getCharCount()) apd();
				crtline_ento = ++pos.y;
				crtcol_ento = pos.x = 1; //_Default_Row_or_Col;
				CrtTType = tok_any;
			}
			else if (method_directive &&
				c == method_directive &&
				(TnodeGetExtnField(*dc.Last())->row != pos.y || CrtTType == tok_any)
				) {
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x - 1;
				CrtTType = tok_direct;
				while ((c = getChar()) != EOF &&
					c != '\n' && c != '\r')
				{
					setChar(c);
				}
				if (c == EOF) break;
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x - 1;
				CrtTType = tok_any;
				backChar(c);
			}

			else if (handler_comment && 1 &&
				backChar(c) && handler_comment(self, true)) {
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x - 1;
				handler_comment(self, false);
				CrtTType = tok_comment;
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x - 1;
				CrtTType = tok_any;
			}
			
			else if (method_string_double_quote && c == '\"' ||
				method_string_single_quote && c == '\'')// enter string
			{
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x - 1;
				CrtTType = tok_string;
				YoString = 1;
				strtok = c;
			}
			else if (
				ascii_isdigit(c) && handler_numchk &&
				 CrtTType != tok_identy)
			{
				if (CrtTType != tok_any) {
					apd();
					crtline_ento = pos.y;
					crtcol_ento = pos.x - 1;
				}
				backChar(c);
				stduint lens = handler_numchk(self);
				// ploginfo("%s handler_numchk> %[u]", __FUNCIDEN__, lens);
				//
				CrtTType = tok_number;
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x;
				CrtTType = tok_any;
				continue;
			}
			else if (ascii_isalnum(c) ||
				method_treatiden_underscore && c == '_')
			{
				if (CrtTType == tok_identy || CrtTType == tok_any)
				{
					setChar(c);
					if (CrtTType == tok_any) CrtTType = tok_identy;
				}
				else
				{
					apd();
					crtline_ento = pos.y;
					crtcol_ento = pos.x - 1;
					setChar(c);
					CrtTType = getctype(c);
				}
			}
			else if (CrtTType == getctype(c) || CrtTType == tok_any)
			{
				setChar(c);
				if (CrtTType == tok_any) CrtTType = getctype(c);
			}
			else {
				apd();
				crtline_ento = pos.y;
				crtcol_ento = pos.x - 1;
				setChar(c);
				CrtTType = getctype(c);
			}
		}
		if (linebuf->getCharCount()) apd();
		return dc;
	}

}

