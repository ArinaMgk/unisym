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

#ifndef _INC_TNODE
#define _INC_TNODE

#include "dnode.h"

#ifdef _INC_CPP
namespace uni {
#endif

	typedef struct TokenParseUnit {
		tchain_t tchn;
		int (*getnext)(void);
		void (*seekback)(stdint chars);
		//
		stduint crtline;
		stduint crtcol;
		char* buffer, * bufptr;
	} TokenParseUnit;

	_CALL_C void StrTokenAll(TokenParseUnit* tpu);

#ifdef _INC_CPP
}
#endif

#ifdef _INC_CPP
namespace uni {
	
	
	struct TokenParseManager {
		/*TODO
			DchainInit
			DchainDrop
		*/

		Dchain dc;
		TokenParseManager(int (*getnext)(void), void (*seekback)(stdint chars), char* buffer) : dc() {
			in_tpu.getnext = getnext;
			in_tpu.seekback = seekback;
			in_tpu.crtcol = in_tpu.crtline = 1;
			in_tpu.buffer = buffer;
			in_tpu.bufptr = buffer;
			dc.func_free;////
		}
		void TokenParse() { 
			StrTokenAll(&in_tpu);
		}
	protected:
		//{TEMP} now C++ is powerful but C, so we use red tapes.
		TokenParseUnit in_tpu;// for token
	};

}
#endif
#endif
