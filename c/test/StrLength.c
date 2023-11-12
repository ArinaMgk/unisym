// RFX09
// @dosconio

#include <stdio.h>
#include <time.h>
#include <string.h>

static char buf[100] = { 0 };
static char bf2[100] = { 0 };
static char str1[] = "a";
static char str2[] = "This is a test text, and it is a middle-length text. This will test the abilities of the string length function.";

/* Implement of GLIBC 2.36
int
STRCMP (const char *p1, const char *p2)
{
  const unsigned char *s1 = (const unsigned char *) p1;
  const unsigned char *s2 = (const unsigned char *) p2;
  unsigned char c1, c2;

  do
    {
      c1 = (unsigned char) *s1++;
      c2 = (unsigned char) *s2++;
      if (c1 == '\0')
	return c1 - c2;
    }
  while (c1 == c2);

  return c1 - c2;
}
libc_hidden_builtin_def (strcmp)
*/

// FROM USTRING.H
static inline size_t StrLengthI(const char* s)
{
	// do not judge s zo null for better debug
	register size_t len = 0;
	while (s[len]) len++;
	return len;
}

__fastcall unsigned StrLength(const char* str);


int main()
{
	double t_0, t_1, t_2, t_3;
	int tmp;
	for (int i = 0; i < sizeof(buf) - 1; i++)
		bf2[i] = 1;
	t_0 = clock();
	// 1. try by glibc
	for (int i = 0; i < 0x1000000; i++)
		buf[i % 100] = strlen(str1);
	for (int i = 0; i < 0x1000000; i++)
		buf[i % 100] = strlen(str2);
	for (int i = 0; i < 0x1000000; i++)
		buf[i % 100] = strlen(bf2);
	t_1 = clock();

	// 2. try by unisym (inline) macro
	for (int i = 0; i < 0x1000000; i++)
		buf[i % 100] = StrLengthI(str1);
	for (int i = 0; i < 0x1000000; i++)
		buf[i % 100] = StrLengthI(str2);
	for (int i = 0; i < 0x1000000; i++)
		buf[i % 100] = StrLengthI(bf2);
	t_2 = clock();

	// 3. try by unisym asm-implement
	for (int i = 0; i < 0x1000000; i++)
		buf[i % 100] = StrLength(str1);
	for (int i = 0; i < 0x1000000; i++)
		buf[i % 100] = StrLength(str2);
	for (int i = 0; i < 0x1000000; i++)
		buf[i % 100] = StrLength(bf2);
	t_3 = clock();
	
	//
	printf("\nThe cost of GLIBC is %lfms\n", (t_1 - t_0) * 1000 / CLOCKS_PER_SEC);
	printf("The cost of USTRING is %lfms\n", (t_2 - t_1) * 1000 / CLOCKS_PER_SEC);
	printf("The cost of USTRING_ASM is %lfms\n", (t_3 - t_2) * 1000 / CLOCKS_PER_SEC);
	// return 0;
}

/* out
	The cost of GLIBC is 472.000000ms
	The cost of USTRING is 1242.000000ms
	The cost of USTRING_ASM is 2176.000000ms
*/

