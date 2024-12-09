
#include "../../inc/c/consio.h"

#if defined(_WinNT) || defined(_Linux)
namespace uni {
	Console_t Console;
	
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
