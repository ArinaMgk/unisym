// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ11
// AllAuthor: @ArinaMgk(till RFA03) @dosconio
// ModuTitle: Assert
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

#ifndef _INC_UASSERT
#define _INC_UASSERT

#define assert(expression) ((expression)?(char*)(expression):(exit(1),(char*)0))

//{} Advanced
// - __FILE__  __BASE_FILE__ __LINE__ __FUNCIDEN__

#endif // !_INC_UASSERT
