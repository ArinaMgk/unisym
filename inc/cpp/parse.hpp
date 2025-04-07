// ASCII C/C++ TAB4 CRLF
// Docutitle: Token Node Parse
// Codifiers: @dosconio: RFZ22 ~
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

#ifndef _INC_PARSE_X
#define _INC_PARSE_X

#include "trait/XstreamTrait.hpp"
#include "nnode"
#include "../c/graphic.h"

namespace uni {

	class LinearParser;
	// Input begun with 0~9, not after {_, a~z, A~Z}
	typedef stduint(*LineParse_NumChk_t)(LinearParser& lp);

	stduint LineParse_NumChk_Default(LinearParser& lp);

	typedef bool(*LineParse_Comment_t)(LinearParser& lp, bool just_chk);

	bool LineParse_Comment_Sharp(LinearParser& lp, bool just_chk);// # ...
	bool LineParse_Comment_C(LinearParser& lp, bool just_chk);// /* ... */
	bool LineParse_Comment_Cpp(LinearParser& lp, bool just_chk);// // ...

	// Take over TokenParseManager
	class LinearParser
	{
		IstreamTrait* src;
		String* buf = 0;
		String* tobuf = 0;
		Point pos = { 1, 1 };
		byte static_lnbuf[byteof(String)];// use if buf null
		byte static_tobuf[byteof(String)];// use if buf null
	public:
		int getChar();
		inline int askChar() { return src->inn(); }
		void setChar(char c) { linebuf->operator<<(c); }
		bool backChar(char c) { (*tobuf) << c; pos.x--; return true; }
	private:
		String* linebuf;
		String* todobuf;

		bool handler_escape_sequence();
	public:
		bool GHT = true;

		// number
		LineParse_NumChk_t handler_numchk = LineParse_NumChk_Default;

		// comment
		LineParse_Comment_t handler_comment = LineParse_Comment_Sharp;

		// string
		bool method_string_single_quote = false;
		bool method_string_double_quote = false;
		bool method_string_escape_sequence = true;

		// directive (do not add comment)
		char method_directive = '\0';// e.g. '%' in "%include"

		// space
		bool method_omit_spaces = true;

		// symbol
		bool method_treatiden_underscore = true;// else, treat as symbol
	public:
		// [1] use heap string
		LinearParser(IstreamTrait& lparser);
		// [2] use buffer string
		LinearParser(IstreamTrait& lparser, char* addr, stduint buflen);
		// [3] use exist string
		LinearParser(IstreamTrait& lparser, String& buf);
		//
		~LinearParser() {
			asserv(buf)->~String();
			if (buf) memf(buf);
			buf = NULL;
			asserv(tobuf)->~String();
			if (tobuf) memf(tobuf);
			tobuf = NULL;
		}
		// take over previous StrTokenAll, deal with literals
		Dchain& Parse(Dchain&);
		// TODO: bool Match(tok_type, &nod)
		// TODO: bool Match(fmt, &{...}) = scanf
		// TODO: bool MatchInteger(&..., signed, base, maxlen); ...

		static void RemoveComments(Dchain& tchain) {
			if (Dnode* crttn = tchain.Root()) while (crttn =
				(crttn->type == tok_comment ? tchain.Remove(crttn) : crttn->next));
		}
		static void StringConcatenation(Dchain& tchain) {
			if (Dnode* crttn = tchain.Root()) do {
				while ((crttn->type == tok_string) && (crttn->next) && (crttn->next->type == tok_string))
				{
					srs(crttn->offs, StrHeapAppend(crttn->addr, crttn->next->addr));
					tchain.Remove(crttn->next);
				}
			} while (crttn = crttn->next);
		}
	};


	class NestedParser {
	public:
		NestedParser(const LinearParser& lparser);


		Nchain& Parse(Nchain&, const Dchain&);
	};


}

#endif
