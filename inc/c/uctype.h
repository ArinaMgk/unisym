// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ26
// AllAuthor: @dosconio
// ModuTitle: Classic C Type Judgement and Conversion
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

#ifndef _INC_UCTYPE
#define _INC_UCTYPE

#define ascii_isdigit(c) ((c)-'0'<10)

#define ascii_islower(c) ((c)-'a'<26)

#define ascii_isupper(c) ((c)-'A'<26)

#define ascii_isalpha(c) ((c)-'A'<26||(c)-'a'<26)

#define ascii_isalnum(c) ((c)-'0'<10||(c)-'A'<26||(c)-'a'<26)

#define ascii_iscontrol(c) ((c)<32||(c)==127)

#define ascii_isprint(c) ((c)>=' '&&(c)<127)

#define ascii_isgraph(c) ((c)>' '&&(c)<127)

#define ascii_ispunct(c) ((c)!=' '&&(c)!='\t'&&(c)!='\n'&&(c)!='\r'&&(c)!='\v'&&(c)!='\f'&&(c)>='!'&&(c)<='~')

#define ascii_isspace(c) ((c)==' '||(c)=='\t'||(c)=='\n'||(c)=='\r'||(c)=='\v'||(c)=='\f')

#define ascii_isblank(c) ((c)==' '||(c)=='\t')

#define ascii_isxdigit(c) ((c)-'0'<10||(c)-'A'<6||(c)-'a'<6)

// ---- ---- ---- ----
extern const unsigned char _tab_tolower[];
extern const unsigned char _tab_toupper[];

#define ascii_tolower(c) (_tab_tolower[(byte)c]) //((c)-'A'<26?(c)|0x20:c)

#define ascii_toupper(c) (_tab_toupper[(byte)c]) //((c)-'a'<26?(c)&~0x20:c)

#define ascii_tohexad(c) ((c)>='a'?(c)-'a'+10:(c)>='A'?(c)-'A'+10 :(c)-'0')

#endif
