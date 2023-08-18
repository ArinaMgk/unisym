#include<stddef.h>
#include<stdio.h>
#include<stdlib.h>
#include<inttypes.h>
#include"../ustring.h"

size_t malc_count;
int main()
{
	char* str;
	StrPoolInit();
	// atexit(StrPoolRelease);
	// printf("%s", str = StrPoolHeap("Hello,world!", 13));
	str = StrPoolHeap("Hello,world!", 4096);
	char* aa = StrPoolHeap("123", 4096);
	char* bb = StrPoolHeap("123456789", 0);
	printf("%llx %llx %llx", str, aa, bb);// 64-bit limit
	printf("\n [ %llx]\n", malc_count);
	StrPoolRelease();
	printf("\n [ %llx]\n", malc_count);
}
