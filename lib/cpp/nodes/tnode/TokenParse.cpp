// ASCII C++-20 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: 20240312
// AllAuthor: @dosconio
// ModuTitle: Token Node
// Depend-on: Tnode.cpp
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

#include "../../../../inc/cpp/cinc"
#include "../../../../inc/c/aldbg.h"
#include "../../../../inc/c/tnode.h"
#include "../../../../inc/cpp/cinc"
#include "../../../../inc/cpp/tnode"
#include <new>
#include <stdio.h>// for EOF
#include <ctype.h>// for isalnum() etc.

// It may be better to use Regular Expression
const char static EscSeq[] = 
{
	'n','\n','r','\r','a','\a','b','\b','f','\f','t','\t','v','\v'
};// other equal to literal char except the "\x"[0~2] "\"[0~2]

//{}{}{} Combinated with C
static toktype getctype(char chr)// Excluding Number
{
	// is going to be renamed "ctype(char)"
	if (isalnum(chr))
		return tok_identy;
	else if (isspace(chr))
		return tok_spaces;
	else if (ispunct(chr))
		return tok_symbol;// including '_' but should be of identifier
	return tok_others;
}

// return whether int, float
static stduint StrTokenAll_NumChk(uni::TokenParseUnit& tpu)
{
	size_t res = 0;
	int c;
	int last_zo_num = 0;
	struct OnceOccur
	{
		unsigned bodx : 1;
		unsigned e : 1;
		unsigned dot : 1;
		unsigned sign : 1;
	} OnceO = { 0 };
	char* bufptr = tpu.buffer;
	tpu.crtcol--;
	tpu.Seekpos(-1);
	while ((c = tpu.Getnext()) != EOF)
	{
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
			if (OnceO.sign || !OnceO.e || *(bufptr - 1) != 'e')
				break;
			else
			{
				last_zo_num = 0; OnceO.sign = 1;
			}
		else if (c >= '0' && c <= '9') last_zo_num = 1;
		else break;
		res++;
		*bufptr++ = c;
	}
	tpu.crtcol--;
	if (c != EOF) tpu.Seekpos(-1);
	if (res > 1 && (*(bufptr - 1) == '+' || *(bufptr - 1) == '-' || *(bufptr - 1) == '.'))
	{
		tpu.crtcol--, tpu.Seekpos(-1);
		res--;
	}
	return res;
}

#define _Default_Row_or_Col 1

namespace uni {
	TokenParseUnit::TokenParseUnit(int (*getnext)(void), void (*seekback)(ptrdiff_t chars), char* buffer) {
		using uni::TnodeChain;
		chain = zalcof(TnodeChain);
		new (chain)TnodeChain(true);
		crtline = crtcol = _Default_Row_or_Col;
		this->getnext = getnext;
		this->seekback = seekback;
		this->bufptr = this->buffer = buffer;
	}

	TokenParseUnit::~TokenParseUnit() {
		chain->~TnodeChain();
		memf(chain);
		chain = 0;
	}

	TnodeChain* TokenParseUnit::TokenParse() {
		toktype CrtTType{ tok_any };
		size_t CrtTLen = 0;// = real length minus 1
		stduint crtline_ento = _Default_Row_or_Col, crtcol_ento = _Default_Row_or_Col;
		int YoString = 0;
		char strtok;
		int c;

		// Arinae Traditional Header Guard
		chain->Append("GHTANIRA", 1, CrtTType, 0, 0);

		// alnum & space & symbol
		while ((c = Getnext()) != EOF)
		{
			char dbg_chr = (char)c;
			if (YoString)// Care about CrtTLen and *bufptr especially
			{
				if (strtok == c)// exit
				{
					CrtTType = tok_string;
					buffer[CrtTLen] = 0;
					chain->Append(buffer, CrtTLen + 1, CrtTType, crtline_ento, crtcol_ento);// including terminated-symbol
					crtline_ento = crtline; crtcol_ento = crtcol;
					bufptr = buffer;
					YoString = 0;
					CrtTLen = 0;
					CrtTType = tok_any;
				}
				else if (c == '\\')// escape sequence
				{
					crtcol++, c = getnext();
					if (c == EOF) break;
					if (isdigit(c))
					{
						// octal: nest 0~2 3 numbers at most
						int nest1 = (crtcol++, getnext());
						if (nest1 == EOF) break;
						if (isdigit(nest1))
						{
							int nest2 = (crtcol++, getnext());
							if (nest2 == EOF) break;
							if (isdigit(nest2))
							{
								// \123
								*bufptr++ = (unsigned char)(nest2 - '0' + (nest1 - '0') * 8 + (c - '0') * 8 * 8);
								CrtTLen++;
							}
							else
							{
								// \12;
								*bufptr++ = (unsigned char)((nest1 - '0') + (c - '0') * 8);
								CrtTLen++;
								crtcol--, seekback(-1);
							}
						}
						else
						{
							// \1;
							*bufptr++ = (unsigned char)(c - '0');
							CrtTLen++;
							crtcol--, seekback(-1);
						}
					}
					else if (c == 'x')// nest 0~3 and use isxdigit()
					{
						crtcol++, c = getnext();
						if (c == EOF) break;
						if (isxdigit(c))
						{
							int nest1 = (crtcol++, getnext());
							if (nest1 == EOF) break;
							if (isxdigit(nest1))
							{
								// \x12;
								nest1 = (isdigit(nest1)) ? (nest1 - '0') : (isupper(nest1)) ? (nest1 - 'A' + 10) : (nest1 - 'a' + 10);
								c = (isdigit(c)) ? (c - '0') : (isupper(c)) ? (c - 'A' + 10) : (c - 'a' + 10);
								*bufptr++ = (unsigned char)(c * 16 + nest1);
								CrtTLen++;
							}
							else
							{
								// \x1;
								c = (isdigit(c)) ? (c - '0') : (isupper(c)) ? (c - 'A' + 10) : (c - 'a' + 10);
								*bufptr++ = (unsigned char)(c);
								CrtTLen++;
								crtcol--, seekback(-1);
							}
						}
						else
						{
							// \x; = \0;
							*bufptr++ = 0;
							CrtTLen++;
							crtcol--, seekback(-1);
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
								*bufptr++ = EscSeq[(i << 1) + 1];
								CrtTLen++;
								listlen = 0;
								break;
							}
						}
						if (!listlen) continue;
						*bufptr++ = c;
						CrtTLen++;
					}
				}
				else
				{
					*bufptr++ = c;
					CrtTLen++;
				}
			}
			else if (c == '\n' || c == '\r')
			{
				// CRLF LFCR CR LF
				int cc = (crtcol++, getnext());
				if (cc == '\n' || cc == '\r')
					if (c == cc)
					{
						crtcol--, seekback(-1);
					}
					else;
				else if (cc != EOF) crtcol--, seekback(-1);
				else break;
				if (CrtTLen)
				{
					crtcol--;
					chain->Append(buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
					crtline_ento = crtline; crtcol_ento = crtcol - 1;// -1?
					bufptr = buffer;
					CrtTLen = 0;
				}
				crtline++;
				crtcol = _Default_Row_or_Col;
				CrtTType = tok_any;
			} 
			else if (c == _TNODE_COMMENT || (c == _TNODE_DIRECTIVE && (chain->Last()->row != crtline || CrtTType == tok_any && !chain->Last()->offs)))// Line Comment
			{
				chain->Append(buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
				crtline_ento = crtline; crtcol_ento = crtcol - 1;// -1?
				bufptr = buffer;
				CrtTLen = 0;
				CrtTType = (c == _TNODE_COMMENT) ? tok_comment : tok_direct;
				while ((crtcol++, c = getnext()) != EOF && c != '\n' && c != '\r')
				{
					CrtTLen++;
					*bufptr++ = c;
				}
				if (c == EOF)break;
				chain->Append(buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
				crtline_ento = crtline; crtcol_ento = crtcol - 1;// -1?
				bufptr = buffer;
				CrtTLen = 0;
				CrtTType = tok_any;
				crtcol--, seekback(-1);
			}
			else if (c == '\"' || c == '\'')// enter
			{
				chain->Append(buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
				crtline_ento = crtline; crtcol_ento = crtcol - 1;// -1?
				CrtTLen = 0;
				bufptr = buffer;
				CrtTType = tok_string;
				YoString = 1;
				strtok = c;
			}
			else if ((isdigit(c)) && CrtTType != tok_identy &&
				((CrtTType != tok_any && (chain->Append(buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento)) && (crtline_ento = crtline, crtcol_ento = crtcol)) || CrtTType == tok_any)
				&& (CrtTLen = StrTokenAll_NumChk(*this)))
			{
				CrtTType = tok_number;
				chain->Append(buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
				crtline_ento = crtline; crtcol_ento = crtcol;
				CrtTType = tok_any;
				CrtTLen = 0;// NOTICE!
				bufptr = buffer;
				continue;
			}
			else if (isalnum(c) || c == '_')
			{
				if (CrtTType == tok_identy || CrtTType == tok_any)
				{
					CrtTLen++;// seem a waste after declaring bufptr.
					*bufptr++ = c;
					if (CrtTType == tok_any) CrtTType = tok_identy;
					if (CrtTLen >= malc_limit)// sizeof(array buffer), "=" considers terminated-zero.
					{
						// erro("Buffer Oversize!");
						return 0;
					}
				}
				else
				{
					chain->Append(buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
					crtline_ento = crtline; crtcol_ento = crtcol - 1;// -1?
					CrtTType = getctype(c);
					CrtTLen = 1;
					bufptr = buffer;
					*bufptr++ = c;
				}
			}
			else
			{
				if (CrtTType == getctype(c) || CrtTType == tok_any)
				{
					CrtTLen++;// seem a waste after declaring bufptr.
					*bufptr++ = c;
					if (CrtTType == tok_any) CrtTType = getctype(c);
					if (CrtTLen >= malc_limit)// sizeof(array buffer), "=" considers terminated-zero.
					{
						// erro("Buffer Oversize!");
						return 0;
					}
				}
				else
				{
					chain->Append(buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
					crtline_ento = crtline; crtcol_ento = crtcol - 1;// -1?
					CrtTLen = 1;
					bufptr = buffer;
					*bufptr++ = c;
					CrtTType = getctype(c);
				}
			}
		}
		if (CrtTLen) chain->Append(buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);

		return chain;
	}

}
