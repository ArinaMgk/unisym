// ASCII C++-STD11 TAB4 CRLF
// LastCheck: 20240229
// AllAuthor: @dosconio
// ModuTitle: Stream
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

#include "../../inc/cpp/unisym"

#if defined(_MCCA)
#else
#include "new"
#endif

#include "../../inc/c/consio.h"
#include "../../inc/c/arith.h"

// useless
// #ifdef _DEV_GCC
// #pragma GCC diagnostic push
// //#pragma GCC diagnostic ignored "-Wint-conversion"// C
// #pragma GCC diagnostic ignored "-Wfloat-conversion"// C++
// #pragma GCC diagnostic ignored "-Wconversion"// C++
// #endif

#define pnext(t) para_next(paras, t)
#define outc     OutChar
#define outtxt   out

#define out_integer  OutInteger
#define out_floating OutFloating


// Console: consio.cpp
//    File: ...
//    XART: ...

// -- C++ MSVC64 class static method -- paralist span a stduint, 20250329
// called from static  OstreamTrait::CountFormat
static stduint _CountFormat(const char* fmt, para_list pl) {
	using namespace uni;
	char buf[byteof(OstreamInstance_t)];
	new (buf) OstreamInstance_t;
	OstreamInstance_t& oi = *(OstreamInstance_t*)buf;
	stduint ret = oi.OutFormat(fmt, pl);
	return ret;
}

#define localout out
namespace uni {
	int OstreamTrait::OutFormatPack(const char* fmt, para_list paras) {
		out_count = 0;
		//
		int i;
		byte c;
		char* s;

		//     % +- ent . end (h/l) sym
		unsigned tmp_base = 16;
		char tmp_signed = 0;// +-
		char tmp_percent_feed = 0;
		// 0 float, 1 double, 2 long double
		// -2 byte, -1 short, 0 int, 1 long, 2 long long
		signed char sizlevel = 0;
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
			//{TODO} use sizlevel
			case 'o':
				out_integer(pnext(unsigned), 8, tmp_signed, true, _TODO 0, _TODO false, intlog2_iexpo(byteof(unsigned)));
				break;
			case 'd':
				out_integer(pnext(signed), 10, tmp_signed, true, _TODO 0, _TODO false, intlog2_iexpo(byteof(signed)));
				break;

			// ---- UNSIGNED ----
			case 'b':// alicee extend
				out_integer(pnext(unsigned), 2, false, false, _TODO 0, _TODO false, intlog2_iexpo(byteof(unsigned)));
				break;
			case 'x':
				out_integer(pnext(unsigned), -16, false, false, _TODO 0, _TODO false, intlog2_iexpo(byteof(unsigned)));
				break;
			case 'u':
				out_integer(pnext(unsigned), 10, false, false, _TODO 0, _TODO false, intlog2_iexpo(byteof(unsigned)));
				break;



			case 'f':
				out_floating_0(sizlevel, paras);
				sizlevel = 0;
				break;
			case 'p':
				localout("0x", 2);
				out_integer(pnext(stduint), -16, false, false, __BITS__ / 4, true, intlog2_iexpo(byteof(stduint)));
				break;
			case 's':
				s = pnext(char*);
				if (!s) s = (char*)"(null)";
				outs(s);
				break;
			case '%':
				c = '%';
				localout(&fmt[i], 1);
				break;
			case '[': // alicee extend
				if (!StrCompareN(fmt + i, "[2c]", 4)) // Print 2-byte Character
				{
					uint16 tmp = para_next_u16(paras);
					const char* ptmp = (char*)&tmp;
					outc(ptmp[0]);
					outc(ptmp[1]);
					i += 4 - 1;
				}
				else if (!StrCompareN(fmt + i, "[u]", 3)) // Print Decimal STDUINT
				{
					stduint uuu = pnext(stduint);
					out_integer(uuu, 10, false, false, 0, false, intlog2_iexpo(byteof(stduint)));
					i += 3 - 1;
				}
				else if (!StrCompareN(fmt + i, "[x]", 3)) // Print Decimal STDUINT
				{
					stduint uuu = pnext(stduint);
					out_integer(uuu, 16, false, false, byteof(stduint) << 1, true, intlog2_iexpo(byteof(stduint)));
					i += 3 - 1;
				}
				else if (!StrCompareN(fmt + i, "[i]", 3)) // Print Decimal STDSINT
				{
					out_integer(pnext(stdsint), 10, false, true, 0, false, intlog2_iexpo(byteof(stdsint)));
					i += 3 - 1;
				}
				else if (!StrCompareN(fmt + i, "[8H]", 4)) // Print Hex STDUINT 8 bit
				{
					out_integer(para_next_u8(paras), 16, false, false, byteof(uint8) << 1, true, intlog2_iexpo(byteof(uint8)));
					i += 4 - 1;
				}
				else if (!StrCompareN(fmt + i, "[16H]", 5)) // Print Hex STDSINT 16 bit
				{
					out_integer(para_next_u16(paras), 16, false, false, byteof(uint16) << 1, true, intlog2_iexpo(byteof(uint16)));
					i += 5 - 1;
				}
				else if (!StrCompareN(fmt + i, "[32H]", 5)) // Print Hex STDSINT 32 bit
				{
					out_integer(pnext(uint32), 16, false, false, byteof(uint32) << 1, true, intlog2_iexpo(byteof(uint32)));
					i += 5 - 1;
				}
				#if defined(_BIT_SUPPORT_64)
				else if (!StrCompareN(fmt + i, "[64H]", 5)) // Print Hex STDSINT 64 bit
				{
					out_integer(pnext(uint64), 16, false, false, byteof(uint64) << 1, true, intlog2_iexpo(byteof(uint64)));
					i += 5 - 1;
				}
				else if (!StrCompareN(fmt + i, "[64I]", 5)) {
					out_integer(pnext(sint64), 10, false, true, 0, true, intlog2_iexpo(byteof(sint64)));
					i += 5 - 1;
				}
				#endif
				break;
			default:
				localout(&fmt[i], 1);
				break;
			}
			if (tmp_percent_feed != 1) {
				tmp_signed = 0;
			}
			i++;
		}

		return out_count;
	}
	int OstreamTrait::OutFormat(const char* fmt, ...) {
		Letpara(paras, fmt);
		auto ret = OutFormatPack(fmt, paras);
		para_endo(paras);
		return ret;
	}
	stduint OstreamTrait::CountFormat(const char* fmt, ...) {
		Letpara(paras, fmt);
		count_mode = true;
		stduint ret = OutFormatPack(fmt, paras);
		count_mode = false;
		para_endo(paras);
		return ret;
	}

	bool OstreamTrait::OutInteger(uint64 val, int base, bool sign_show, bool sign_have, byte least_digits, bool zero_padding, byte bytexpo)
	{
		asrtret(base >= 2 || base == -16);
		// ---- COMMON ----
		char buf[bitsof(val) + 1] = { 0 };// asci_zero 
		char* bufp = buf + sizeof(buf) - 1;
		bool neg = sign_have && (*(stdsint*)&val) < 0;
		char pad = zero_padding ? '0' : ' ';
		char* alnum_tab = base == -16 ? _tab_hexa : _tab_HEXA;
		if (neg) {
			val = ~val + 1;
			sign_show = 1;
		}
		if (_IMM1S(bytexpo) < byteof(stduint)) // use if to avoid MSVC ROL discovered at 20250227
			val &= _IMM1S(_IMM1S(bytexpo + 3)) - 1;// <=> case 0: val = (uint8)val; break ...
		if (base < 0) base = -base;
		#if defined(_MCCA) && (_MCCA == 0x1032 || _MCCA == 0x1064)
		uint64 valquo = 0, valrem = 0;
		do {
			valquo = udivmoddi4(val, base, &valrem);
			*--bufp = _tab_HEXA[valrem];
		} while (val = valquo);
		#else
		do *--bufp = _tab_HEXA[val % base]; while (val /= base);
		#endif
		// ---- PRINT ----
		if (sign_show) out(neg ? "-" : "+", 1);
		stdsint num_to_print = StrLength(bufp);
		if (least_digits > num_to_print) for0(i, least_digits - num_to_print) out(&pad, 1);
		out(bufp, num_to_print);
		return true;

	}

	#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
	__attribute__((target("sse2")))
		#endif
		void OstreamTrait::OutFloating(double val) {
		if (val < 0) { localout("-", 1); val = -val; }
		out_integer((stduint)val, 10, false, false, 0, true, intlog2_iexpo(byteof(stduint)));
		val -= (stduint)val;
		val *= 1000000;
		val += 0.5;
		if (_IMM(val)) {
			stduint afterdot_digits = 6;
			stduint v = val;
			while (afterdot_digits > 0 && !(v % 10)) {
				v /= 10;
				afterdot_digits--;
			}
			localout(".", 1);
			out_integer(v, 10, false, false, afterdot_digits, true, intlog2_iexpo(byteof(stduint)));
		}
	}
	#if defined(_MCCA) && ((_MCCA & 0xFF00) == 0x8600)
	__attribute__((target("sse2")))
	#endif
	void OstreamTrait::out_floating_0(stduint sizlevel, para_list paras) {
		if (sizlevel == 1)
			out_floating(pnext(double));
		else if (sizlevel == 0)
			out_floating(para_next_float(paras));
	}



}

// #ifdef _DEV_GCC
// #pragma GCC diagnostic pop
// #endif
