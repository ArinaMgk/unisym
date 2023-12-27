// RFX11 x64 MSVC2022
#define _AUTO_INCLUDE
#include <stdio.h>
#include "../ustring.h"
#include <time.h>

static inline void* MemSetByte(void* s, int c, size_t n)
{
	while (n) { n--; ((char*)s)[n] = (char)c; }
	return s;
}

static inline void* MemSetWord(void* s, int c, size_t n)
{
	register union { byte* bptr;  word* wptr; size_t val; } ptr;
	register word wrd;
	byte remain;
	if (!n) return s;
	ptr.bptr = (byte*)s;
	if (ptr.val & 1) { *ptr.bptr++ = c; n--; if (!n) return s; }
	remain = n & 1;
	wrd = (word)((byte)c) << 8 | (byte)c;
	n >>= 1;
	do// loop
	{
		*ptr.wptr++ = wrd;// STOSW
	} while (--n);
	if (remain) *ptr.bptr++ = (byte)c;// STOSB
	return s;
}

static inline void* MemSetDword(void* s, int c, size_t n)
{
	register union { byte* bptr;  word* wptr; dword* dptr; size_t val; } ptr;
	register dword dwd;
	byte remain;
	if (!n) return s;
	ptr.bptr = (byte*)s;
	while (ptr.val & 0b11) { *ptr.bptr++ = c; n--; if (!n) return s; }
	remain = n & 0b11;
	dwd = (dword)((byte)c) << 24 | (dword)((byte)c) << 16 | (dword)((byte)c << 8) | (byte)c;
	n >>= 2;
	do// loop
	{
		*ptr.dptr++ = dwd;// STOSD
	} while (--n);
	if (remain) do *ptr.bptr++ = (byte)c; while (--remain);// STOSB
	return s;
}

int main()
{
	double d0, d1;
	char buf[45] = { 0 };

	printf("Current bits: %u %u\n", __BITS__, sizeof(size_t)*8);
	
	d0 = clock();
	for (int i = 0; i < 0x1000000; i++)
		MemSetByte(buf, 'A', 44);
	d1 = clock();
	printf("%lf\n", d1 - d0);
	puts(buf);

	d0 = clock();
	for (int i = 0; i < 0x1000000;i++) 
		MemSetWord(buf, 'A', 44);
	d1 = clock();
	printf("%lf\n", d1 - d0);
	puts(buf);

	d0 = clock();
	for (int i = 0; i < 0x1000000; i++)
		MemSetDword(buf, 'A', 44);
	d1 = clock();
	printf("%lf\n", d1 - d0);
	puts(buf);

	d0 = clock();
	for (int i = 0; i < 0x1000000; i++)
		MemSet(buf, 'A', 44);// x64
	d1 = clock();
	printf("%lf\n", d1 - d0);
	puts(buf);
}
/* OUTPUT MSVC2022 x64
	890.000000
	AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
	570.000000
	AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
	418.000000
	AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
	415.000000
	AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

memset:
	58.000000
	AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
*/

/* GCC w64devkit/i686-w64-mingw32
	Current bits: 32 32
	1156.000000
	AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
	421.000000
	AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
	215.000000
	AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
	214.000000
	AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
*/
