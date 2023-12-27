// ASCII C99 TAB4 CRLF
// Attribute: General
// LastCheck: RFZ17
// AllAuthor: @dosconio
// ModuTitle: Version Information
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

#include "../../../inc/c/alice.h"

const char __PROJ_INFO__[] =
{""
#ifdef __PROJNAME__
"Project: " __PROJNAME__ "\n"
#endif
#ifdef __PROJVERS__
"Version: " __PROJVERS__ "\n"
#endif
#ifdef __BUILDENV__
"Build  : " __DATE__     "\n"
"Environ: " __BIT_STR__   " bit\n"
#endif
#ifdef __AUTHOR__
"Author : " __AUTHOR__   "\n"
#endif
#ifdef __LICENSE__
"License: " __LICENSE__  "\n"
#endif
};
