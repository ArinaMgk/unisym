// ASCII RFC12&13 Haruno
#include <stdio.h>
extern char* ChrAddx64(const char* a, const char* b);
int main()
{
	char* p;
	char num_a[4] = "12";
	char num_b[4] = "99";
	p = ChrAddx64(num_a, num_b);
	printf("12+99=%s\n", p);

	char num_c[6] = "2";
	char num_d[6] = "2999";
	p = ChrAddx64(num_c, num_d);
	printf("2+2999=%s\n", p);

	return 0;
}

