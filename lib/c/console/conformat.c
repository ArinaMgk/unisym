// ASCII C99 TAB4 CRLF
// Docutitle: (Module) Console Input and Output
// Codifiers: @ArinaMgk; @dosconio: 20231116 ~ <Last-check> 
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
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

#define _INC_USTDBOOL
#define bool int
#define boolean byte
#define BOOLEAN byte

#include "../../../inc/c/consio.h"
#include "../../../inc/c/ISO_IEC_STD/stdlib.h"

void outc(const char chr)
{
	outtxt(&chr, 1);
}

//{TEMP} always align to right
void outi8hex(const byte inp)
{
	byte val = inp;
	char buf[2];
	for (stduint i = 0; i < 2; i++)
	{
		buf[1 - i] = _tab_HEXA[val & 0xF];
		val >>= 4;
	}
	outtxt(buf, 2);
}

// Replacement of DbgEcho16
void outi16hex(const word inp)
{
	word val = inp;
	char buf[4];
	for (stduint i = 0; i < 4; i++)
	{
		buf[3 - i] = _tab_HEXA[val & 0xF];
		val >>= 4;
	}
	outtxt(buf, 4);
}

// Replacement of DbgEcho32
void outi32hex(const dword inp)
{
	dword val = inp;
	char buf[8];
	for (stduint i = 0; i < 8; i++)
	{
		buf[7 - i] = _tab_HEXA[val & 0xF];
		val >>= 4;
	}
	outtxt(buf, 8);
}

void outi64hex(const uint64 inp)
{
	uint64 val = inp;
	char buf[16];
	for (stduint i = 0; i < numsof(buf); i++)
	{
		buf[numsof(buf) - 1 - i] = _tab_HEXA[val & 0xF];
		val >>= 4;
	}
	outtxt(buf, numsof(buf));
}

void outidec(int xx, int base, int sign)
{
	char buf[16];
	int i;
	unsigned x;

	if (sign && (sign = xx < 0))
		x = -xx;
	else
		x = xx;

	i = 0;
	do {
		buf[i++] = _tab_HEXA[x % base];
	} while ((x /= base) != 0);

	if (sign)
		buf[i++] = '-';

	while (--i >= 0)
		outc(buf[i]);
}

void outi(stdint val, int base, int sign_show)
{
	if (base < 2) return;
	char buf[bitsof(stdint)] = { 0 };
	int i = 0;
	if (val < 0) val = -val;
	do buf[i++] = _tab_HEXA[val % base]; while (val /= base);
	if (sign_show) buf[i++] = val < 0 ? '-' : '+';
	outtxt(buf, numsof(buf));
}

int outsfmtlst(const char* fmt, para_list paras)
{
	int i;
	byte c;
	char* s;
	// para_list paras = lst;
	if (fmt == 0) return 0;

	for (i = 0; (c = fmt[i]); ) {
		if (c != '%') {
			stduint len = 1;
			const char* q = fmt + i;
			while (q[len] && (q[len] != '%')) len++;
			outtxt(&fmt[i], len);
			i += len;
			continue;
		}
		c = fmt[++i];
		if (c == 0)
			break;
		switch (c) {
		case 'd':
			outidec(para_next(paras, int), 10, 1);
			break;
		case 'x':
			outidec(para_next(paras, int), 16, 1);
			break;
		case 'p':
			outtxt("0x", 2);
			if (bitsof(stduint) == 64)
				outi64hex(para_next(paras, stduint));
			else if (bitsof(stduint) == 32)
				outi32hex(para_next(paras, stduint));
			else if (bitsof(stduint) == 16)
				outi16hex(para_next(paras, stduint));
			else
				outi8hex(para_next(paras, stduint));
			break;
		case 's':
			if ((s = para_next(paras, char*)) == 0)
				s = "(null)";
			outtxt(s, -1);
			break;
		case '%':
			c = '%';
			outtxt(&fmt[i], 1);
			break;
		default:
			// no-care
			break;
		}
		i++;
	}
	return i;
}

//{TEMP} only understands %d, %x, %p, %s with-no modification.
int outsfmt(const char* fmt, ...)
{
	Letpara(args, fmt);
	return outsfmtlst(fmt, args);
	// para_endo(args);
}
