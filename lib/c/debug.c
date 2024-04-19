// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant CPU(80586+)
// LastCheck: RFZ26
// AllAuthor: @dosconio
// ModuTitle: Debug Aide
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

#include "../../inc/c/aldbg.h"

#ifdef _DEBUG
size_t malc_count = 0;
size_t malc_limit = _MALLIMIT_DEFAULT;// you can use such `malc_limit = 100;` to change this.
size_t call_state;
#endif
