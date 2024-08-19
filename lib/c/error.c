// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant 
// LastCheck: RFZ23
// AllAuthor: @dosconio
// ModuTitle: Error processing
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

#include "../../inc/c/msgface.h"
#include "../../inc/c/stdinc.h"
#include "../../inc/c/ISO_IEC_STD/stdlib.h"
#include <stdio.h>

#if defined(_DEBUG) || defined(_dbg)

// Node* _WARN_CHAIN = NULL;
char* _ERRO_MESSAGE = NULL;
jmp_buf _ERRO_JUMP = { 0 };

void _cast_panic() {
	// make use of jmp_buf to jump to potential handler
	while (1);
	//abort();
}

void erro(char* erromsg)
{
	_ERRO_MESSAGE = erromsg;
	if (_ERRO_JUMP) longjmp(_ERRO_JUMP, 1);
	else
	{
		fprintf(stderr, "Error: %s\n", erromsg);
		memf(_ERRO_MESSAGE);
		exit(1);
	}
}

void warn(char* warnmsg)
{
	/*
	if (_WARN_CHAIN) NodeAppend(_WARN_CHAIN, warnmsg);
	else _WARN_CHAIN = NodeAppend(NULL, warnmsg);
	*///{TODO}
}

#endif

