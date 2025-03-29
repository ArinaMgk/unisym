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
		buf = new String;
	}
	LinearParser::LinearParser(IstreamTrait& lparser, char* addr, stduint buflen) {
		buf = NULL;// or static_buf
		new (static_buf) String(addr, buflen);
	}
	LinearParser::LinearParser(IstreamTrait& lparser, String& buf) {
		this->buf = &buf;
	}

	int LinearParser::getChar() {
		return _TODO - 1;
	}
	void LinearParser::moveCursor(stdsint relative) {

	}

	stduint LineParse_NumChk_Default(LinearParser& lp) {
		return _TODO nil;
	}

	stduint LineParse_Comment_Sharp(LinearParser& lp) {
		return _TODO nil;
	}
#if 0
	Dchain& LinearParser::Parse(Dchain dc) {
		toktype CrtTType = tok_any;
		size_t CrtTLen = 0;// = real length minus 1
		stduint crtline_ento = _Default_Row_or_Col, crtcol_ento = _Default_Row_or_Col;
		int YoString = 0;
		char strtok;
		int c;
		tpu->bufptr = tpu->buffer;
		char* const buffer = tpu->buffer;

		tpu->tchn.func_free = DnodeHeapFreeSimple;
		// Traditional Header Guard
		StrTokenAppend(&tpu->tchn, "GHTANIRA", 1, tok_any, 0, 0);

		// alnum & space & symbol
		while ((c = this_getn(tpu)) != EOF)
		{
			char dbg_chr = (char)c;
			if (YoString)// Care about CrtTLen and *buf-ptr especially
			{
				if (strtok == c)// exit
				{
					CrtTType = tok_string;
					buffer[CrtTLen++] = 0;
					apd();// including terminated-symbol
					crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol;
					YoString = 0;
					CrtTLen = 0;
					CrtTType = tok_any;
				}
				else if (c == '\\')// escape sequence
				{
					c = this_getn(tpu);
					if (c == EOF) break;
					if (ascii_isdigit(c))
					{
						// octal: nest 0~2 3 numbers at most
						int nest1 = this_getn(tpu);
						if (nest1 == EOF) break;
						if (ascii_isdigit(nest1))
						{
							int nest2 = this_getn(tpu);
							if (nest2 == EOF) break;
							if (ascii_isdigit(nest2))
							{
								// \123
								*tpu->bufptr++ = (unsigned char)(nest2 - '0' + (nest1 - '0') * 8 + (c - '0') * 8 * 8);
								CrtTLen++;
							}
							else
							{
								// \12;
								*tpu->bufptr++ = (unsigned char)((nest1 - '0') + (c - '0') * 8);
								CrtTLen++;
								this_back(tpu);
							}
						}
						else
						{
							// \1;
							*tpu->bufptr++ = (unsigned char)(c - '0');
							CrtTLen++;
							this_back(tpu);
						}
					}
					else if (c == 'x')// nest 0~3 and use isxdigit()
					{
						c = this_getn(tpu);
						if (c == EOF) break;
						if (ascii_isxdigit(c))
						{
							int nest1 = this_getn(tpu);
							if (nest1 == EOF) break;
							if (ascii_isxdigit(nest1))
							{
								// \x12;
								nest1 = (ascii_isdigit(nest1)) ? (nest1 - '0') : (ascii_isupper(nest1)) ? (nest1 - 'A' + 10) : (nest1 - 'a' + 10);
								c = (ascii_isdigit(c)) ? (c - '0') : (ascii_isupper(c)) ? (c - 'A' + 10) : (c - 'a' + 10);
								*tpu->bufptr++ = (unsigned char)(c * 16 + nest1);
								CrtTLen++;
							}
							else
							{
								// \x1;
								c = (ascii_isdigit(c)) ? (c - '0') : (ascii_isupper(c)) ? (c - 'A' + 10) : (c - 'a' + 10);
								*tpu->bufptr++ = (unsigned char)(c);
								CrtTLen++;
								this_back(tpu);
							}
						}
						else
						{
							// \x; = \0;
							*tpu->bufptr++ = 0;
							CrtTLen++;
							this_back(tpu);
						}
					}
					else
					{
						// escape char and other
						if (c == EOF) break;
						size_t listlen = sizeof(EscSeq) / 2;
						for (size_t i = 0; i < listlen; i++)
						{
							if (EscSeq[i << 1] == c)
							{
								*tpu->bufptr++ = EscSeq[(i << 1) + 1];
								CrtTLen++;
								listlen = 0;
								break;
							}
						}
						if (!listlen) continue;
						*tpu->bufptr++ = c;
						CrtTLen++;
					}
				}
				else
				{
					*tpu->bufptr++ = c;
					CrtTLen++;
				}
			}
			else if (c == '\n' || c == '\r')
			{
				// CRLF LFCR CR LF
				int cc = this_getn(tpu);
				if (cc == '\n' || cc == '\r')
					if (c == cc)
					{
						this_back(tpu);
					}
					else;
				else if (cc != EOF) this_back(tpu);
				else break;
				if (CrtTLen)
				{
					tpu->crtcol--;
					apd();
					crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol - 1;// -1?
					CrtTLen = 0;
				}
				tpu->crtline++;
				tpu->crtcol = _Default_Row_or_Col;
				CrtTType = tok_any;
			}
			else if (c == _TNODE_COMMENT || (c == _TNODE_DIRECTIVE && (TnodeGetExtnField(*tpu->tchn.last_node)->row != tpu->crtline || CrtTType == tok_any && !tpu->tchn.last_node->offs)))// Line Comment
			{
				apd();
				crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol - 1;// -1?
				CrtTLen = 0;
				CrtTType = (c == _TNODE_COMMENT) ? tok_comment : tok_direct;
				while ((c = this_getn(tpu)) != EOF && c != '\n' && c != '\r')
				{
					CrtTLen++;
					*tpu->bufptr++ = c;
				}
				if (c == EOF)break;
				apd();
				crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol - 1;// -1?
				CrtTLen = 0;
				CrtTType = tok_any;
				this_back(tpu);
			}
			else if (c == '\"' || c == '\'')// enter
			{
				apd();
				crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol - 1;// -1?
				CrtTLen = 0;
				CrtTType = tok_string;
				YoString = 1;
				strtok = c;
			}
			else if ((ascii_isdigit(c)) && CrtTType != tok_identy &&
				((CrtTType != tok_any && apd() && (crtline_ento = tpu->crtline, crtcol_ento = tpu->crtcol)) || CrtTType == tok_any)
				&& (CrtTLen = StrTokenAll_NumChk(tpu)))
			{
				CrtTType = tok_number;
				apd();
				crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol;
				CrtTType = tok_any;
				CrtTLen = 0;// NOTICE!
				continue;
			}
			else if (ascii_isalnum(c) || c == '_')
			{
				if (CrtTType == tok_identy || CrtTType == tok_any)
				{
					CrtTLen++;// seem a waste after declaring tpu->bufptr.
					*tpu->bufptr++ = c;
					if (CrtTType == tok_any) CrtTType = tok_identy;
					if (CrtTLen >= malc_limit)// sizeof(array buffer), "=" considers terminated-zero.
					{
						// erro("Buffer Oversize!");
					}
				}
				else
				{
					apd();
					crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol - 1;// -1?
					CrtTType = getctype(c);
					CrtTLen = 1;
					*tpu->bufptr++ = c;
				}
			}
			else
			{
				if (CrtTType == getctype(c) || CrtTType == tok_any)
				{
					CrtTLen++;// seem a waste after declaring tpu->bufptr.
					*tpu->bufptr++ = c;
					if (CrtTType == tok_any) CrtTType = getctype(c);
					if (CrtTLen >= malc_limit)// sizeof(array buffer), "=" considers terminated-zero.
					{
						// erro("Buffer Oversize!");
					}
				}
				else
				{
					apd();
					crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol - 1;// -1?
					CrtTLen = 1;
					*tpu->bufptr++ = c;
					CrtTType = getctype(c);
				}
			}
		}
		if (CrtTLen) apd();


		return dc;
	}
#endif
}

