// UTF-8 C99 TAB4 CRLF
// LastCheck: RFX17
// AllAuthor: @ArinaMgk
// ModuTitle: Unisym Practical EnCoding

#include <stdio.h>
#include "../../../inc/c/upec.h"

int main()
{
	for(int i = 0x20; i < 0x7F; i++)
	{
		printf("%c %c\n", i, upec2ascii(ascii2upec(i)));
	}
	printf("TAB %d %d\n", '\t', upec2ascii(ascii2upec('\t')));
	printf("CR %d %d\n", '\r', upec2ascii(ascii2upec('\r')));
	printf("LF %d %d\n", '\n', upec2ascii(ascii2upec('\n')));
	printf("upper a is %c\n", upec2ascii(touupper(ascii2upec('a'))));
	printf("lower A is %c\n", upec2ascii(toulower(ascii2upec('A'))));
}
