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

#ifndef _INC_ERROR
#define _INC_ERROR

// #include "node.h"

#include "stdinc.h"

#define errjb _ERRO_JUMP

// comments-in content will conflict with GLIBC, independent!
// extern Node* _WARN_CHAIN;
// extern Node* _FREE_CHAIN;// tofree list
extern char* _ERRO_MESSAGE;
// extern jmp_buf _ERRO_JUMP;

#if defined(_DEBUG) || defined(_dbg)

//
#define init_total_errmech(exitcode) \
	if (setjmp(errjb))\
	{\
		if (_ERRO_MESSAGE && *_ERRO_MESSAGE)\
			fprintf(stderr, "!Err %s\n", _ERRO_MESSAGE);\
		if(exitcode&&_MALCOUNT) fprintf(stderr, "MEMORY LEAK %" PRIuPTR " TIMES.\n", _MALCOUNT);\
		return (exitcode);\
	}

//
void erro(char* erromsg);

//
void warn(char* warnmsg);

void _cast_panic();

#endif

// GNU::stddef.h)
#if defined(_DEV_GCC)
#define _CRT_ERRNO_DEFINED
#ifdef _WinNT
 __attribute__((__dllimport__))
#endif
extern int* __attribute__((__cdecl__)) _errno(void);
#define errno (*_errno())
#endif


#endif // !_INC_ERROR
