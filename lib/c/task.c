// ASCII CPL TAB4 CRLF
// Docutitle: Task
// Datecheck: 20240421 ~ <Last-check>
// Developer: @dosconio
// Attribute: <ArnCovenant> <Environment> <Platform>
// Reference: <Reference>
// Dependens: <Dependence>
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

// take this as reference and there is no need or standard to use this directly.

#include "../../inc/c/task.h"
#include "../../inc/c/ustring.h"

#if defined(_ARC_x86)
#if _ARC_x86 >= 3

#include "../../inc/c/format/ELF.h"

//{outdated}
// LocaleDescriptor32SetFromELF32
// 0
// 1 code: 00000000~7FFFFFFF
// 2 data: 00000000~7FFFFFFF
// 3
// 4 ss-0: 00000000~FFFFFFFF
// 5 ss-1: 00000000~FFFFFFFF
// 6 ss-2: 00000000~FFFFFFFF
// 7 ss-3: 00000000~7FFFFFFF
#define FLAT_CODE_R3_PROP 0x00C7FA00
#define FLAT_DATA_R3_PROP 0x00C7F200
#define FLAT_DATA_R2_PROP 0x00CFD200
#define FLAT_DATA_R1_PROP 0x00CFB200
#define FLAT_DATA_R0_PROP 0x00CF9200
void TaskFlatRegister(TaskFlat_t* TaskFlat, descriptor_t* GDT)
{
	TSS_t* TSS = TaskFlat->TSS;
	TSS->LastTSS = TaskFlat->parent;
	TSS->NextTSS = 0;
	TSS->ESP0 = TaskFlat->esp0;
	TSS->SS0 = 8 * 4 + 4 + 0;// 4:LDT 8*4:SS0 0:Ring0 
	TSS->Padding0 = 0;
	TSS->ESP1 = TaskFlat->esp1;
	TSS->SS1 = 8 * 5 + 4 + 1;// 4:LDT 8*5:SS1 1:Ring1
	TSS->Padding1 = 0;
	TSS->ESP2 = TaskFlat->esp2;
	TSS->SS2 = 8 * 6 + 4 + 2;// 4:LDT 8*6:SS2 2:Ring2
	TSS->Padding2 = 0;
	TSS->CR3 = getCR3(); //{TODO} general paging mechanism
	TSS->EIP = TaskFlat->entry;
	TSS->EFLAGS = getEflags();
	TSS->EAX = TSS->ECX = TSS->EDX = TSS->EBX = TSS->EBP = TSS->ESI = TSS->EDI = 0;
	switch (TaskFlat->ring)
	{
	case 0: TSS->ESP = TaskFlat->esp0;
		break;
	case 1: TSS->ESP = TaskFlat->esp1;
		break;
	case 2: TSS->ESP = TaskFlat->esp2;
		break;
	default: 
	case 3: TSS->ESP = TaskFlat->esp3;
			break;	
	}
	//{TODO} below - adapt for non-ring3 case
	//{TODO} allow IOMap Version
	TSS->ES = 8*2 + 7;
	TSS->Padding3 = 0;
	TSS->CS = 8*1 + 7;
	TSS->Padding4 = 0;
	TSS->SS = 8*7 + 7;
	TSS->Padding5 = 0;
	TSS->DS = TSS->ES;
	TSS->Padding6 = 0;
	TSS->FS = TSS->ES;
	TSS->Padding7 = 0;
	TSS->GS = TSS->ES;
	TSS->Padding8 = 0;
	TSS->LDTDptr = (TaskFlat->LDTSelector * 8) + 3; // LDT yo GDT
	TSS->LDTLength = 8 * 8 - 1;
	TSS->STRC_15_T = 0;
	TSS->IO_MAP = sizeof(TSS_t) - 1; // TaskFlat->IOMap? TaskFlat->IOMap-TSS: sizeof(TSS_t)-1;
	// Then register LDT in GDT
	GlobalDescriptor32Set(&GDT[TaskFlat->LDTSelector], (dword)TaskFlat->LDT, TSS->LDTLength, _Dptr_LDT, 0, 0 /* is_sys */, 1 /* 32-b */, 0 /* not-4k */	);// [0x00408200]
	GlobalDescriptor32Set(&GDT[TaskFlat->TSSSelector], (dword)TaskFlat->TSS, sizeof(TSS_t)-1, _Dptr_TSS386_Available, 0, 0 /* is_sys */, 1 /* 32-b */, 0 /* not-4k */ );// TSS [0x00408900]

	dword* ldt_alias = (dword*)TaskFlat->LDT;
	ldt_alias[0] = 0x00000000;
	ldt_alias[1] = 0x00000000;

	ldt_alias[2] = 0x0000FFFF;
	ldt_alias[3] = FLAT_CODE_R3_PROP;

	ldt_alias[4] = 0x0000FFFF;
	ldt_alias[5] = FLAT_DATA_R3_PROP;

	// kept for RONL
	ldt_alias[6] = 0x00000000;
	ldt_alias[7] = 0x00000000;

	ldt_alias[8]  = 0x0000FFFF;
	ldt_alias[9]  = FLAT_DATA_R0_PROP;
	ldt_alias[10] = 0x0000FFFF;
	ldt_alias[11] = FLAT_DATA_R1_PROP;
	ldt_alias[12] = 0x0000FFFF;
	ldt_alias[13] = FLAT_DATA_R2_PROP;
	ldt_alias[14] = 0x0000FFFF;
	ldt_alias[15] = FLAT_DATA_R3_PROP;
	// EXPERIENCE: Although the stack segment is not Expand-down, the ESP always decreases. The property of GDTE is just for boundary check.
	/*
	normal stack:
		ADD EAX, ECX; STACK BASE FROM HIGH END
		MOV EBX, 0xFFFFE; 4K
		MOV ECX, 00C09600H; [4KB][32][KiSys][DATA][R0][RW^/RE^]
	*/
}

#endif
#endif
