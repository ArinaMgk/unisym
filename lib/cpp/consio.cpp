
#include "../../inc/c/consio.h"
#include <stdio.h>

#if defined(_WinNT) || defined(_Linux)
namespace uni {
	HostConsole Console;

	int HostConsole::out(const char* str, dword len) {
		_crt_out_cnt = nil;
		outtxt(str, len);
		return _crt_out_cnt;
	}
	int HostConsole::inn() {
		return _TEMP getchar();
	}

	int HostConsole::FormatShow(const char* fmt, ...) {
		Letpara(args, fmt);
		outbyte_t last = outredirect(outtxt);
		int ret = outsfmtlst(fmt, args);
		outredirect(last);
		return ret;
	}
	
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
