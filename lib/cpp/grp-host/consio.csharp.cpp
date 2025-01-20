// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant yo Host
// LastCheck: 2023 Nov 16
// AllAuthor: @ArinaMgk since RFR30; @dosconio
// ModuTitle: Console Input and Output
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

#define _STYLE_CSHARP

#include "../../../inc/c/consio.h"

#ifdef _MCCA
#define _NEWLINE "\n\r"
#elif defined(_Linux)
#define _NEWLINE "\n"
#elif defined(_WinNT)
#define _NEWLINE "\r\n"
#elif defined(_MacOS)
#define _NEWLINE "\r"
#endif

#ifdef _NEWLINE
namespace uni {

	int HostConsole::WriteLine(const char* fmt, ...) {
		Letpara(args, fmt);
		outbyte_t last = outredirect(outtxt);
		int ret = outsfmtlst(fmt, args);
		if (ret >= 0) ret += outsfmt(_NEWLINE);
		outredirect(last);
		return ret;
	}
	int HostConsole::WriteLine(const String& str) {
		outbyte_t last = outredirect(outtxt);
		int ret = outsfmt("%s", str.reference());
		if (ret >= 0) ret += outsfmt(_NEWLINE);
		outredirect(last);
		return ret;
	}
	
}
#endif
