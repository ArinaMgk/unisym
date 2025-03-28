﻿// ASCII C/C++ TAB4 CRLF
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

#ifndef _INC_TNODE
#define _INC_TNODE

#include "dnode.h"
#include "ustring.h"

#ifdef _INC_CPP
namespace uni {
#endif

	typedef struct TokenParseUnit {
		tchain_t tchn;
		int (*getnext)(void);// user-def
		void (*seekback)(stdint chars);// user-def
		//
		stduint crtline;
		stduint crtcol;
		//{OUTDATED} using LinearParser
		char* buffer,// user-def
			* bufptr;
	} TokenParseUnit;// C Style

	// Must and All chars begun with 0~9
	typedef size_t(*StrTokenAll_NumChk_t)(TokenParseUnit* tpu);

	typedef enum TokenParseOption_e {
		string_single_quote = 0x01,
		string_double_quote = 0x02,
		comment_line_after = 0x04,// like "//"
		directive = 0x08,// like # in "#include"
		omit_spaces = 0x10,
	} TokenParseOption_e;

	typedef struct TokenParseMethod {
		TokenParseOption_e option;
		StrTokenAll_NumChk_t numchk;

	} TokenParseMethod;

	_ESYM_C size_t StrTokenAll_NumChk(TokenParseUnit* tpu);

	_ESYM_C stduint StrTokenNormal(TokenParseUnit* tpu, const TokenParseMethod* method);

	// TODO: bool Match(tok_type, &nod)
	// TODO: bool Match(fmt, &{...}) = scanf
	// TODO: bool MatchInteger(&..., signed, base, maxlen); ...

	_ESYM_C void StrTokenAll(TokenParseUnit* tpu);

#ifdef _INC_CPP
}
#endif

#ifdef _INC_CPP
namespace uni {


	struct TokenParseManager {
		Dchain dc;
		TokenParseManager(int (*getnext)(void), void (*seekback)(stdint chars), char* buffer) : dc() {
			in_tpu.getnext = getnext;
			in_tpu.seekback = seekback;
			in_tpu.crtcol = in_tpu.crtline = 1;
			in_tpu.buffer = buffer;
			in_tpu.bufptr = buffer;
			//
			DchainInit(&in_tpu.tchn);
			in_tpu.tchn.extn_field = sizeof(uni::TnodeField);
			dc.func_free = 0;//{TODO}
			inntpu_avail = outtpu_avail = false;
		}
		~TokenParseManager() {
			//{TODO}
		}
		bool TokenParse() {
			StrTokenAll(&in_tpu);
			inntpu_avail = true;
			uni::Dnode* crt = in_tpu.tchn.root_node;
			if (crt) do if (crt->type != tok_spaces) {
				dc.Append(crt->offs, false)->type = crt->type;
			}
			else {
				memf(crt->offs);
			}
			while (crt = crt->next);
			in_tpu.tchn.func_free = NULL;
			DchainDrop(&in_tpu.tchn);
			inntpu_avail = false;
			outtpu_avail = true;
			return true;
		}
	protected:
		//{TEMP} now C++ is powerful but C, so we use red tapes.
		TokenParseUnit in_tpu;// for token
		bool inntpu_avail;
		bool outtpu_avail;
	};

}
#endif
#endif
