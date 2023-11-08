// ASCII RFX-05 C-99 ArnCovenant TAB4 CRLF
#define _dbg
#define _AUTO_INCLUDE
#include "../aldbg.h"
#include "../stack.h"
#include <inttypes.h>
#include <stdio.h>

size_t malc_count;

void funktion()
{
	printf("\nThe stack is full!");
}

void finish()
{
	stack_free(STACK_GLOBAL);
	if (malc_count) printf("\n%" PRIdPTR "\n", malc_count);
	puts("Oyasuminasaiii~");
}

int main()
{
	int i = 1;
	char buf[] = { "Hello, Unisym!" };

	atexit(finish);

	printb(!push(i));
	printb(!pop(i));

	puts(" ==== ==== ==== ====");
	stack_init(STACK_GLOBAL, 10);
	stack_global_event_full(funktion);
	for (i = 0; i <= StrLength(buf); i++)
		printb(!push(buf[i]));

	puts(" ==== ==== ==== ====");
	for (i = 0; i < StrLength(buf) + 2; i++)
	{
		char tmp;
		if (!pop(tmp))
			printf("%c", tmp);
		else printf("\nThe stack is empty!");
	}
}
