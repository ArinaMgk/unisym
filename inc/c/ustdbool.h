// ASCII C99/C++ TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ11
// AllAuthor: @ArinaMgk(till RFA03) @dosconio
// ModuTitle: Universal Standard bool : binary or logical type
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

#ifndef _INC_USTDBOOL
#define _INC_USTDBOOL

#define immed_tobool(i) !!(i) // != 0

#if !(defined(__cplusplus) || defined(_INC_CPP)) && \
	(defined(_DEV_MSVC) || defined(__STDC_VERSION__) && __STDC_VERSION__ < 202300L)// Solve new-GCC appended error.
typedef enum boolean { false, true } bool, boolean;// use immed_tobool() to convert with this
#else
typedef bool boolean;
#endif

#endif
