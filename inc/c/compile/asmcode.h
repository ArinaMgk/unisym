// ASCII C-C99 TAB4 CRLF
// Docutitle: (Module) ASM-used Literal Instructions and Directives
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
// Attribute: <ArnCovenant> Any-Architect <Environment> <Reference/Dependence>
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

#ifndef _INC_ASMCODE
#define _INC_ASMCODE

//{TEMP} Style: Intel-ASM for x86 and GAS-ASM for RISC-V

#define _INST_SUFFIX "\n"

static const char *_AUT_HEAD[] = {
	"; ASCII AASM TAB4 LF" _INST_SUFFIX,
	"# ASCII GAS TAB4 LF" _INST_SUFFIX,
};
static const char *_AUT_ARCHITECT[] = {
	"; Architect: Intel 80x86" _INST_SUFFIX,
	"# Architect: RISC-V 64" _INST_SUFFIX,
};
//{TODO} directive like `[CPU 386]`
static const char *_ASM_GLOBAL[] = {
	"GLOBAL main ; default entry" _INST_SUFFIX,
	".global main # default entry" _INST_SUFFIX,
};
static const char *_ASM_RET[] = {
	"RET" _INST_SUFFIX,
	"ret" _INST_SUFFIX,
};
static const char *_ASM_MOV[] = {
	"MOV EAX, %ld" _INST_SUFFIX,
	"li a0, %ld" _INST_SUFFIX,
};
static const char *_ASM_ADD[] = {
	"ADD EAX, %ld" _INST_SUFFIX,
	"add a0, a0, %ld" _INST_SUFFIX,
};


#endif
