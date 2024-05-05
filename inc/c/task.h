// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant yo Host
// LastCheck: 20240313
// AllAuthor: @dosconio
// ModuTitle: x86 Task Module
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

#ifndef _INC_TASK
#define _INC_TASK

#include "stdinc.h"

#if defined(_ARC_x86)
#if _ARC_x86 >= 3

typedef struct TaskStateSegmentx86
{
	word LastTSS;
	word NextTSS;// Mecocoa's Design
	dword ESP0;
	word SS0;
	word Padding0;
	dword ESP1;
	word SS1;
	word Padding1;
	dword ESP2;
	word SS2;
	word Padding2;
	dword CR3;// Page Directory Base
	dword EIP;
	dword EFLAGS;
	dword EAX;
	dword ECX;
	dword EDX;
	dword EBX;
	dword ESP;
	dword EBP;
	dword ESI;
	dword EDI;
	word ES;
	word Padding3;
	word CS;
	word Padding4;
	word SS;
	word Padding5;
	word DS;
	word Padding6;
	word FS;
	word Padding7;
	word GS;
	word Padding8;
	word LDTDptr;
	word LDTLength;// Mecocoa's Design
	word STRC_15_T;
	word IO_MAP; // default 103
	// ---- 0104d:
} TSS_t;

typedef struct
{
	word LDTSelector;
	word TSSSelector;
	word parent;
	descriptor_t* LDT;
	TSS_t* TSS;
	byte ring: 2;
	dword esp0, esp1, esp2, esp3;
	dword entry;
	void* IOMap;//{unused} //[Com-Covenant] usually after TSS in memory
	stduint TSSBlockLength;//{unused} from beginning off TSS to end of IOMap
} TaskFlat_t;

void TaskFlatRegister(TaskFlat_t* TaskFlat, descriptor_t* GDT);

#endif
#else
//
#endif // _ARC_x86
#endif
