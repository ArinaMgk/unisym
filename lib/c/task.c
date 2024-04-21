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

#include "../../inc/c/task.h"

#if defined(_ARC_x86)
#if _ARC_x86 >= 3

TSS_t* TaskStateSegmentInitialize(TSS_t* tss)
{
	byte* tmp = (byte*)tss;
	for0(i, byteof(*tss))
		tmp[i] = 0;
	return tss;
}

// based on n_demos::TSSStruct_3
void TaskStateSegmentSet(TSS_t* tss)
{
	//TODO
}

#endif
#endif
