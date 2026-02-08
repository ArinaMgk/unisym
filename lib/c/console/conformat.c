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

#include "../../../inc/c/consio.h"
#include "../../../inc/c/arith.h"
#include "../../../inc/c/ISO_IEC_STD/stdlib.h"

#define pnext(t) para_next(paras, t)

#define out_integer(a1,a2,a3,a4,a5,a6,a7) outinteger(a1,a2,a3,a4,a5,a6,a7, local_out)
#define out_floating(a1) outfloat(a1, local_out)

// [Single Thread]
// keep the 4 not nested!
// 1> Screen / Device{Console...}
// 1> Buffer / Stream{File...}
// 2> String.Format(...) : (1)getlen (2)getval
// can be empty fucntion but nullptr
static outbyte_t local_out = outtxt;
_TODO byte local_out_lock = 0;// 0 for accessable
stduint _crt_out_cnt;
stduint _crt_out_lim;// for StringN Series, ~0 for no limit

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
//DEF_outiXhex(8);
// Replacement of DbgEcho16
//DEF_outiXhex(16);
// Replacement of DbgEcho32
//DEF_outiXhex(32);
//
//DEF_outiXhex(64);


#if defined(_BIT_SUPPORT_64)
bool outinteger(uint64 val, int base, bool sign_show, bool sign_have, byte least_digits, bool zero_padding, byte bytexpo, outbyte_t out)
{
	#include "../../../inc/c/stream/format-out-integer.h"
}
#else
#endif

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
__attribute__((target("sse2")))
#endif
static void outfloat(float val, outbyte_t localout)
{
	#include "../../../inc/c/stream/format-out-floating.h"
}

#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
__attribute__((target("sse2")))
#endif
static void out_floating_0(stduint sizlevel, para_list paras) {
	if (sizlevel == 1)
		out_floating(pnext(double));
	else if (sizlevel == 0)
		out_floating(para_next_float(paras));
}

int outsfmtlst(const char* fmt, para_list paras)
{
	// while (local_out_lock);
	// local_out_lock = 1;//{TODO} add multitask lock
	outbyte_t localout = local_out;
	_crt_out_cnt = 0;
	#define _STREAM_FORMAT_C
	#include "../../../inc/c/stream/format-body.h"
	// _TEMP local_out_lock = 0;// leave lock
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
