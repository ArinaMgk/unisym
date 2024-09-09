// ASCII DJGPP (Debug MSVC2010) TAB4 CRLF
// Attribute: ArnCovenant Host bin^16+ 386+
// LastCheck: RFX19
// AllAuthor: @dosconio
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

#define __PROJNAME__ "Ain Assembler of Arinae UNISYM"
#define __PROJVERS__ "0101H"
#define __AUTHOR__ "@dosconio"
#define __LICENSE__ "Apache License 2"

#include <stdinc.h>

_TEMP//:

typedef struct {
	//: Called at the start of a pass
	//{TODO} void (*reset) (char*, int, efunc, evalfunc, ListGen*, StrList**);
	//: Called to fetch a line of preprocessed source.
	_Need_free char* (*getline) (void);
	//: Called at the end of a pass.
	void (*cleanup) (int);
} Prepro;
extern Prepro _pp;
