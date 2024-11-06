// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Module) 
// Codifiers: @dosconio: 20240906 ~ <Last-check> 
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

#ifndef _INC_STRBUFF
#define _INC_STRBUFF

#include "stdinc.h"
#include <stdio.h>

typedef struct StrBuff {
	stduint elm_len;
	stduint blk_len;
	stduint blk_cnt; // allocated
	stduint blk_p_cnt; // allocated
	stduint alloc_len;
	stduint total_len;
	struct {// write
		char** p;
		stduint p_inn;
		stduint p_glb;
	};
	struct {// read
		char** q;
		stduint q_inn;
		stduint q_glb;
	};
	char** blks_ptrs;
} Strbuff;

Strbuff* StrbuffNew(stduint elm_len);

void StrbuffFree(Strbuff* s);

pureptr_t StrbuffEalloc(Strbuff* s);

void StrbuffSend(Strbuff* s, pureptr_t src, stduint len);

void StrbuffRewind(Strbuff* s);

pureptr_t StrbuffEread(Strbuff* s);

const void* StrbuffRead(Strbuff* s, stduint* lenp);

void StrbuffPop(Strbuff* s, pureptr_t dest, stduint len);

void StrbuffPopx(Strbuff* s, stduint posi, pureptr_t dest, stduint len);

void StrbuffSendx(Strbuff* s, stduint posi, const pureptr_t src, stduint len);

void StrbuffSendFile(Strbuff* s, FILE* fp);

void StrbuffSend8(Strbuff* s, uint8 v);
void StrbuffSend16(Strbuff* s, uint16 v);
void StrbuffSend32(Strbuff* s, uint32 v);
#ifdef _BIT_SUPPORT_64
void StrbuffSend64(Strbuff* s, uint64 v);
#endif
void StrbuffSendStd(Strbuff* s, stduint v);

void StrbuffSendWleb128u(Strbuff* s, int value);

void StrbuffSendWleb128s(Strbuff* s, int value);

#endif
