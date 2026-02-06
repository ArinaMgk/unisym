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

#if !defined(_MCCA)
#include <stdio.h>
#endif

#if defined(_DEBUG) || defined(_dbg)

// Node* _WARN_CHAIN = NULL;
Retpoint _ERRO_JUMP;

void _cast_panic() {
	while (1);
	//abort();
}

void erro(const char* erromsg)
{
	if (&_ERRO_JUMP) JumpPoint(&_ERRO_JUMP, (pureptr_t)erromsg);
	else
	{
		printlog(_LOG_ERROR, "%s", erromsg);
		// memf(erromsg);//{TODO} a leak here
		exit(1);
	}
}

void warn(const char* warnmsg)
{
	/*
	if (_WARN_CHAIN) NodeAppend(_WARN_CHAIN, warnmsg);
	else _WARN_CHAIN = NodeAppend(NULL, warnmsg);
	*///{TODO}
}

#endif

