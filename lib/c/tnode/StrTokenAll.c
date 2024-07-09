// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ22
// AllAuthor: @dosconio
// ModuTitle: Token Node
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


#include "../../../inc/c/dnode.h"
#include "../../../inc/c/uctype.h"
#include "../../../inc/c/ustring.h"
#include <stdio.h>// for EOF

#define _TNODE_COMMENT '#'
#define _TNODE_DIRECTIVE '%'

// static size_t crtline = 0, crtcol = 0;

// It may be better to use Regular Expression
const char static EscSeq[] = 
{
	'n','\n','r','\r','a','\a','b','\b','f','\f','t','\t','v','\v'
};// other equal to literal char except the "\x"[0~2] "\"[0~2]

static toktype getctype(char chr)// Excluding Number
{
	// is going to be renamed "ctype(char)"
	if (ascii_isalnum(chr))
		return tok_identy;
	else if (ascii_isspace(chr))
		return tok_spaces;
	else if (ascii_ispunct(chr))
		return tok_symbol;// including '_' but should be of identifier
	return tok_others;
}

static int this_getn(TokenParseUnit* tpu) {
	tpu->crtcol++;
	return tpu->getnext();
}

static void this_back(TokenParseUnit* tpu) {
	tpu->crtcol--;
	tpu->seekback(-1);
}

static size_t StrTokenAll_NumChk(TokenParseUnit* tpu)
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
	tpu->crtcol--;
	tpu->seekback(-1);
	while ((c = this_getn(tpu)) != EOF)
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
			if (OnceO.sign || !OnceO.e || *(tpu->bufptr - 1) != 'e')
				break;
			else
			{
				last_zo_num = 0; OnceO.sign = 1;
			}
		else if (c >= '0' && c <= '9') last_zo_num = 1;
		else break;
		res++;
		*tpu->bufptr++ = c;
	}
	tpu->crtcol--;
	if (c != EOF) tpu->seekback(-1);
	if (res > 1 && (*(tpu->bufptr - 1) == '+' || *(tpu->bufptr - 1) == '-' || *(tpu->bufptr - 1) == '.'))
	{
		tpu->crtcol--, tpu->seekback(-1);
		res--;
	}
	return res;
}


#define _Default_Row_or_Col 1



void StrTokenAll(TokenParseUnit* tpu)
{
	toktype CrtTType = tok_any;
	size_t CrtTLen = 0;// = real length minus 1
	stduint crtline_ento = _Default_Row_or_Col, crtcol_ento = _Default_Row_or_Col;
	int YoString = 0;
	char strtok;
	int c;
	char* const buffer = tpu->buffer;

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
				buffer[CrtTLen] = 0;
				StrTokenAppend(&tpu->tchn, buffer, CrtTLen + 1, CrtTType, crtline_ento, crtcol_ento);// including terminated-symbol
				crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol;
				tpu->bufptr = buffer;
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
				StrTokenAppend(&tpu->tchn, buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
				crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol - 1;// -1?
				tpu->bufptr = buffer;
				CrtTLen = 0;
			}
			tpu->crtline++;
			tpu->crtcol = _Default_Row_or_Col;
			CrtTType = tok_any;
		}
		else if (c == _TNODE_COMMENT || (c == _TNODE_DIRECTIVE && (TnodeGetExtnField(*tpu->tchn.last_node)->row != tpu->crtline || CrtTType == tok_any && !tpu->tchn.last_node->offs)))// Line Comment
		{
			StrTokenAppend(&tpu->tchn, buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
			crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol - 1;// -1?
			tpu->bufptr = buffer;
			CrtTLen = 0;
			CrtTType = (c == _TNODE_COMMENT) ? tok_comment : tok_direct;
			while ((c = this_getn(tpu)) != EOF && c != '\n' && c != '\r')
			{
				CrtTLen++;
				*tpu->bufptr++ = c;
			}
			if (c == EOF)break;
			StrTokenAppend(&tpu->tchn, buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
			crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol - 1;// -1?
			tpu->bufptr = buffer;
			CrtTLen = 0;
			CrtTType = tok_any;
			this_back(tpu);
		}
		else if (c == '\"' || c == '\'')// enter
		{
			StrTokenAppend(&tpu->tchn, buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
			crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol - 1;// -1?
			CrtTLen = 0;
			tpu->bufptr = buffer;
			CrtTType = tok_string;
			YoString = 1;
			strtok = c;
		}
		else if ((ascii_isdigit(c)) && CrtTType != tok_identy &&
			((CrtTType != tok_any && StrTokenAppend(&tpu->tchn, buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento) && (crtline_ento = tpu->crtline, crtcol_ento = tpu->crtcol)) || CrtTType == tok_any)
			&& (CrtTLen = StrTokenAll_NumChk(tpu)))
		{
			CrtTType = tok_number;
			StrTokenAppend(&tpu->tchn, buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
			crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol;
			CrtTType = tok_any;
			CrtTLen = 0;// NOTICE!
			tpu->bufptr = buffer;
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
				StrTokenAppend(&tpu->tchn, buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
				crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol - 1;// -1?
				CrtTType = getctype(c);
				CrtTLen = 1;
				tpu->bufptr = buffer;
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
				StrTokenAppend(&tpu->tchn, buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
				crtline_ento = tpu->crtline; crtcol_ento = tpu->crtcol - 1;// -1?
				CrtTLen = 1;
				tpu->bufptr = buffer;
				*tpu->bufptr++ = c;
				CrtTType = getctype(c);
			}
		}
	}
	if (CrtTLen) StrTokenAppend(&tpu->tchn, buffer, CrtTLen, CrtTType, crtline_ento, crtcol_ento);
}
