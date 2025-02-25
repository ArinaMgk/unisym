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

#include "../../inc/c/consio.h"
#if !defined(_MCCA)
#include <stdio.h>
#endif

#if defined(_WinNT) || defined(_Linux) ||  defined(_MCCA)
namespace uni {
	HostConsole Console;

	int HostConsole::out(const char* str, stduint len) {
		::_crt_out_cnt = nil;
		outtxt(str, len);
		return ::_crt_out_cnt;
	}
	int HostConsole::inn() {
	#if defined(_WinNT) || defined(_Linux)
		return _TEMP getchar();
	#else
		return _TEMP 0;
	#endif
	}

#if 0 // outdated
	int HostConsole::FormatShow(const char* fmt, ...) {
		Letpara(args, fmt);
		outbyte_t last = outredirect(outtxt);
		int ret = outsfmtlst(fmt, args);
		outredirect(last);
		return ret;
	}
#endif

}
#endif

#if 0

size_t ConScanLine(char* innbuf, size_t limit) {
	char ch;
	while (i < limit && std::cin.get(ch) && ch != '\n')
		innbuf[i++] = ch;
	innbuf[i] = 0;
}

#endif
