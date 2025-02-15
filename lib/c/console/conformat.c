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

#define pnext(t) para_next(paras, t)

// [Single Thread]
// keep the 4 not nested!
// 1> Screen / Device{Console...}
// 1> Buffer / Stream{File...}
// 2> String.Format(...) : (1)getlen (2)getval
// can be empty fucntion but nullptr
static outbyte_t local_out = outtxt;
Handler_t _serial_callback = 0;
_TODO byte local_out_lock = 0;// 0 for accessable
stduint _crt_out_cnt;

void outc(const char chr)
{
	local_out(&chr, 1);
}

//{TODO} preempt the lock

#undef outs
#define outs(x) local_out(x, StrLength(x))

//{TEMP} always align to right
#define DEF_outiXhex(siz) void outi##siz##hex(uint##siz inp) {\
	outbyte_t localout = local_out;\
	char buf[2 * byteof(uint##siz) + 1] = {0};\
	for0r(i, numsof(buf) - 1) {\
		buf[i] = _tab_HEXA[inp & 0xF];\
		inp >>= 4;\
	}\
	outs(buf);\
}
//
DEF_outiXhex(8);
// Replacement of DbgEcho16
DEF_outiXhex(16);
// Replacement of DbgEcho32
DEF_outiXhex(32);
//
DEF_outiXhex(64);

// [outdated] Output `int` Decimal, not `stduint`
void outidec(int xx, int base, int sign)
{
	char buf[2 * byteof(int) + 2];
	int i;
	unsigned x;
	if (sign && (sign = xx < 0))
		x = -xx;
	else
		x = Castype(unsigned, xx);
	i = 0; do {
		buf[i++] = _tab_HEXA[x % base];
	} while ((x /= base) != 0);
	if (sign)
		buf[i++] = '-';
	while (--i >= 0)
		outc(buf[i]);
}

// Output Integer
void outi(stdint val, int base, int sign_show)
{
	outbyte_t localout = local_out;
	if (base < 2) return;
	char buf[bitsof(stdint) + 2] = { 0 };// may bigger than 2 * bitsof(stdint) + 2 if base < 16 
	int i = sizeof(buf) - 1;
	int neg = val < 0;
	if (neg) {
		val = -val;
		sign_show = 1;
	}
	do buf[--i] = _tab_HEXA[val % base]; while (val /= base);
	if (sign_show) buf[--i] = neg ? '-' : '+';
	outs(buf + i);
}
#if defined(_BIT_SUPPORT_64)
void outi64(int64 val, int base, int sign_show)
{
	outbyte_t localout = local_out;
	if (base < 2) return;
	char buf[bitsof(int64) + 2] = { 0 };
	int i = sizeof(buf) - 1;
	int neg = val < 0;
	if (neg) {
		val = -val;
		sign_show = 1;
	}
	do buf[--i] = _tab_HEXA[val % base]; while (val /= base);
	if (sign_show) buf[--i] = neg ? '-' : '+';
	outs(buf + i);
}
#endif

// Output Unsigned Integer
void outu(stduint val, int base)
{
	outbyte_t localout = local_out;
	if (base < 2) return;
	char buf[bitsof(stduint) + 1] = { 0 };
	stduint i = sizeof(buf) - 1;
	do buf[--i] = _tab_HEXA[val % base]; while (val /= base);
	outs(buf + i);
}

_TEMP static void outfloat(float val)
{
	outbyte_t localout = local_out;
	if (val < 0) localout("-", 1);
	outu((stduint)val, 10);
	val -= (stduint)val;
	val *= 1000000;
	val += 0.5;
	if (_IMM(val)) {
		localout(".", 1);
		outu((stduint)val, 10);
	}
}

int outsfmtlst(const char* fmt, para_list paras)
{
	while (local_out_lock);
	local_out_lock = 1;//{TODO} add multitask lock
	outbyte_t localout = local_out;
	_crt_out_cnt = 0;
	int i;
	byte c;
	char* s;

	//     % +- ent . end (h/l) sym
	unsigned tmp_base = 16;
	char tmp_signed = 0;// +-
	char tmp_percent_feed = 0;
	// 0 float, 1 double, 2 long double
	// -2 byte, -1 short, 0 int, 1 long, 2 long long
	char sizlevel = 0;
	stduint ent = 0;
	stduint end = 0;
	char dotted = 0;

	if (fmt == 0) return 0;

	for (i = 0; tmp_percent_feed || (c = fmt[i]); ) {
		if (c != '%') {
			stduint len = 1;
			const char* q = fmt + i;
			while (q[len] && (q[len] != '%')) len++;
			localout(&fmt[i], len);
			i += len;
			continue;
		}
		if (!tmp_percent_feed) i++; else tmp_percent_feed = 0;
		c = fmt[i];
		if (c == 0)
			break;
		switch (c) {
		case 'c':
			outc(para_next_char(paras));
			break;

			// ---- SIGN & SIZE SWITCH ----
		case '+':// '-' for alignment
			tmp_signed = 1;
			c = '%'; tmp_percent_feed = 1;
			break;
		// '-'
		case 'l':// no mixed with 'h'
			if (sizlevel >= 0) sizlevel++;
			c = '%'; tmp_percent_feed = 1;
			break;
		case 'h':
			if (sizlevel <= 0) sizlevel--;
			c = '%'; tmp_percent_feed = 1;
			break;


			// ---- INTEGER ---- [signed] [base] ...

		case 'b':// alicee extend
			tmp_base = 2;
			goto case_integer;
		case 'o':
			tmp_base = 8;
			goto case_integer;
		case 'd':
			tmp_base = 10;
		case_integer:
			outi(pnext(int), tmp_base, tmp_signed);
			tmp_signed = 0;
			break;
		case 'x':
			tmp_base = 16;
		case_unteger:
			outu(pnext(int), tmp_base);
			break;
		


		case 'f':
			if (sizlevel == 1)
				outfloat(pnext(double));
			else if (sizlevel == 0)
				outfloat(pnext(float));
			sizlevel = 0;
			break;
		case 'p':
			localout("0x", 2);
			// typeid
			#if __BITS__ == 64
				outi64hex(pnext(stduint));
			#elif __BITS__ == 32
				outi32hex(pnext(stduint));
			#elif __BITS__ == 16
				outi16hex(pnext(stduint));
			#elif __BITS__ == 8
				outi8hex(pnext(stduint));
			#else
				#error "Unsupport bits"
			#endif
			break;
		case 's':
			s = pnext(char*);
			if (!s) s = "(null)";
			outs(s);
			break;
		case '%':
			c = '%';
			localout(&fmt[i], 1);
			break;
		case '[': // alicee extend
			if (!StrCompareN(fmt + i, "[2c]", 4)) // Print 2-byte Character
			{
				uint16 tmp = pnext(uint16);
				const char* ptmp = (char*)&tmp;
				outc(ptmp[0]);
				outc(ptmp[1]);
				i += 4 - 1;
			}
			else if(!StrCompareN(fmt + i, "[u]", 3)) // Print Decimal STDUINT
			{
				outu(pnext(stduint), 10);
				i += 3 - 1;
			}
			else if (!StrCompareN(fmt + i, "[i]", 3)) // Print Decimal STDSINT
			{
				stdsint tmp = pnext(stdsint);
				outi(tmp, 10, tmp < 0);
				i += 3 - 1;
			}
			else if (!StrCompareN(fmt + i, "[8H]", 4)) // Print Hex STDUINT 8 bit
			{
				outi8hex(pnext(uint8));
				i += 4 - 1;
			}
			else if (!StrCompareN(fmt + i, "[16H]", 5)) // Print Hex STDSINT 16 bit
			{
				outi16hex(pnext(uint16));
				i += 5 - 1;
			}
			else if (!StrCompareN(fmt + i, "[32H]", 5)) // Print Hex STDSINT 32 bit
			{
				outi32hex(pnext(uint32));
				i += 5 - 1;
			}
			// ...
		#if defined(_BIT_SUPPORT_64)
			// Print Decimal 64 bit
			else if (!StrCompareN(fmt + i, "[64I]", 5)) {
				outi64(pnext(int64), 10, 0);
				i += 5 - 1;
			}
		#endif
			break;
		default:
			localout(&fmt[i], 1);
			break;
		}
		i++;
	}

	_TEMP local_out_lock = 0;// leave lock
	asserv(_serial_callback)();
	return _crt_out_cnt;
}

//{TEMP} only understands %d, %x, %p, %s with-no modification.
int outsfmt(const char* fmt, ...)
{
	Letpara(args, fmt);
	return outsfmtlst(fmt, args);
	// para_endo(args);
}

outbyte_t outredirect(outbyte_t out)
{
	outbyte_t last_out = local_out;
	//{TODO} check lock there
	if (out) local_out = out;
	return last_out;
}
