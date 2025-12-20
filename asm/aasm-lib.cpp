// ASCII C TAB4 CRLF
// Docutitle: (Module) 
// Codifiers: @dosconio: 20241114
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

#define _HIS_TIME_H
#include <c/stdinc.h>
//
#include <c/datime.h>
#include <cpp/string>
#include <c/multichar.h>
//#include <c/uctype.h>
#include <time.h>


#include <insns.h>

_ESYM_C uint64_t _crc64_tab_nasm[256];

struct cpu_list_t { rostr idn; uint32 val; };
cpu_list_t cpu_list[] = {
	{"8086", IF_8086},
	{"186", IF_186},
	{"286", IF_286},
	{"386", IF_386},
	{"486", IF_486},
	{"586", IF_PENT}, {"pentium", IF_PENT},
	{"686", IF_P6}, {"ppro", IF_P6}, {"pentiumpro", IF_P6}, {"p2", IF_P6},
	{"p3", IF_KATMAI}, {"katmai", IF_KATMAI},
	{"p4", IF_WILLAMETTE}, {"willamette", IF_WILLAMETTE},
	{"prescott", IF_PRESCOTT},
	{"x64", IF_X86_64}, {"x86-64", IF_X86_64},
	{"ia64", IF_IA64},
	{"ia-64", IF_IA64},
	{"itanium", IF_IA64},
	{"itanic", IF_IA64},
	{"merced", IF_IA64},
};

// segment-number allocator
extern "C" stdint next_seg = 0;
extern "C" stdint SegAlloc();
extern "C" void SegInit();
stdint SegAlloc() { // SegAlloc
	return (next_seg += 2) - 2;
}
void SegInit() { next_seg = 0; }


// ---- floating handler ----

//{TODO} UNI DATIME
extern "C" void define_macros_early(time_t* startup_time)
{
	char temp[128];
	tm lt, * lt_p, gm, * gm_p;
	int64_t posix_time = 0;

	if (lt_p = localtime(startup_time)) {
		lt = *lt_p;

		strftime(temp, sizeof temp, "__DATE__=\"%Y-%m-%d\"", &lt);
		Preprocessor::pre_define(temp);
		strftime(temp, sizeof temp, "__DATE_NUM__=%Y%m%d", &lt);
		Preprocessor::pre_define(temp);
		strftime(temp, sizeof temp, "__TIME__=\"%H:%M:%S\"", &lt);
		Preprocessor::pre_define(temp);
		strftime(temp, sizeof temp, "__TIME_NUM__=%H%M%S", &lt);
		Preprocessor::pre_define(temp);
	}

	if (gm_p = gmtime(startup_time)) {
		gm = *gm_p;

		strftime(temp, sizeof temp, "__UTC_DATE__=\"%Y-%m-%d\"", &gm);
		Preprocessor::pre_define(temp);
		strftime(temp, sizeof temp, "__UTC_DATE_NUM__=%Y%m%d", &gm);
		Preprocessor::pre_define(temp);
		strftime(temp, sizeof temp, "__UTC_TIME__=\"%H:%M:%S\"", &gm);
		Preprocessor::pre_define(temp);
		strftime(temp, sizeof temp, "__UTC_TIME_NUM__=%H%M%S", &gm);
		Preprocessor::pre_define(temp);
	}

	if (gm_p)
		posix_time = POSIXGetSeconds(&gm);
	else if (lt_p)
		posix_time = POSIXGetSeconds(&lt);

	if (posix_time) {
		Preprocessor::pre_define(uni::String::newFormat("__POSIX_TIME__=%[64I]", posix_time).reflect());
	}
}

extern "C" void define_macros_late(rostr ofmt_shortname)
{
	Preprocessor::pre_define(uni::String::newFormat("__OUTPUT_FORMAT__=%s\n", ofmt_shortname).reflect());
}

// ---- floating handler ----



static void hand_fp_normal(rostr str) {
	//--auto clean flag
	log_pass1 = 1;
	aasm_log(_LOG_ERROR, "too many periods in floating-point constant");
}
#define MANT_DIGITS 52
static void hand_fp_over_digits() {
	//--auto clean flag
	log_pass1 = 1;
	log_warnt = ERR_WARN_FL_TOOLONG;
	aasm_log(_LOG_WARN, "floating-point constant significand contains more than %i digits", MANT_DIGITS);
}
static void hand_fp_1() {
	if (pass0 == 1) {
		//--auto clean flag
		log_pass1 = 1;
		log_warnt = ERR_WARN_FL_OVERFLOW;
		aasm_log(_LOG_WARN, "overflow in floating-point constant");
	}
}
static void hand_FL_UNDERFLOW() {
	//--auto clean flag
	log_pass1 = 1;
	log_warnt = ERR_WARN_FL_UNDERFLOW;
	aasm_log(_LOG_WARN, "underflow in floating-point constant");
}
static void hand_FL_OVERFLOW() {
	//--auto clean flag
	log_pass1 = 1;
	log_warnt = ERR_WARN_FL_OVERFLOW;
	aasm_log(_LOG_WARN, "overflow in floating-point constant");
}
static void hand_FL_DENORM() {
	//--auto clean flag
	log_pass1 = 1;
	log_warnt = ERR_WARN_FL_DENORM;
	aasm_log(_LOG_WARN, "denormal floating-point constant");
}


//

void mainx() {
	fp_normal = hand_fp_normal;
	fp_over_digits = hand_fp_over_digits;
	fp_fn1 = hand_fp_1;
	fp_fn_underflow = hand_FL_UNDERFLOW;
	fp_fn_overflow = hand_FL_OVERFLOW;
	fp_fn_denormal = hand_FL_DENORM;
}



// ---- CRC64 ----
_ESYM_C{
	uint64_t crc64(uint64_t crc, const char* str) {
		foreach_byt(ch, str)
			crc = _crc64_tab_nasm[(byte)crc ^ ch] ^ (crc >> _BYTE_BITS_);
		return crc;
	}
	uint64_t crc64i(uint64_t crc, const char* str) {
		foreach_byt(ch, str)
			crc = _crc64_tab_nasm[(byte)crc ^ ascii_tolower(ch)] ^ (crc >> _BYTE_BITS_);
		return crc;
	}
}

// ---- CPU ----
uint32 get_cpu_id(rostr iden) {
	for0a(i, cpu_list) if (!StrCompareInsensitive(cpu_list[i].idn, iden)) {
		return cpu_list[i].val;
	}
	return IF_PLEVEL;
}

// ---- Char ----
_ESYM_C{
	//{UNCHK} Apply a specific string transform and return it in a nasm_malloc'd buffer, returning the length.  On error, returns (size_t)-1 and no buffer is allocated.
	size_t string_transform(char* str, size_t len, char** out, enum strfunc func)
	{
		if (func == 0)
			return CscUTF(8, 16, str, len, (pureptr_t*)out);
		else if (func == 1)
			return CscUTF(8, 32, str, len, (pureptr_t*)out);
		else return 0;
	}

}

// ---- String Format ----

#define lib_isnumchar(c)   (aasm_isalnum(c) || (c) == '$' || (c) == '_')
#define numvalue(c)  ((c)>='a' ? (c)-'a'+10 : (c)>='A' ? (c)-'A'+10 : (c)-'0')


static int radix_letter(char c)
{
	switch (c) {
	case 'b': case 'B':
	case 'y': case 'Y':
		return 2;		/* Binary */
	case 'o': case 'O':
	case 'q': case 'Q':
		return 8;		/* Octal */
	case 'h': case 'H':
	case 'x': case 'X':
		return 16;		/* Hexadecimal */
	case 'd': case 'D':
	case 't': case 'T':
		return 10;		/* Decimal */
	default:
		return 0;		/* Not a known radix letter */
	}
}
int64_t readnum(char* str, bool* error)
{
	char* r = str, * q;
	int32_t pradix, sradix, radix;
	int plen, slen, len;
	uint64_t result, checklimit;
	int digit, last;
	bool warn = false;
	int sign = 1;

	*error = false;

	while (ascii_isspace(*r)) r++; // find start of number */

	/*
	 * If the number came from make_tok_num (as a result of an %assign), it
	 * might have a '-' built into it (rather than in a preceeding token).
	 */
	if (*r == '-') {
		r++;
		sign = -1;
	}

	q = r;

	while (lib_isnumchar(*q))
		q++;                    /* find end of number */

	len = q - r;
	if (!len) { // Not numeric
		*error = true;
		return 0;
	}

	/*
	 * Handle radix formats:
	 *
	 * 0<radix-letter><string>
	 * $<string>		(hexadecimal)
	 * <string><radix-letter>
	 */
	pradix = sradix = 0;
	plen = slen = 0;

	if (len > 2 && *r == '0' && (pradix = radix_letter(r[1])) != 0)
		plen = 2;
	else if (len > 1 && *r == '$')
		pradix = 16, plen = 1;

	if (len > 1 && (sradix = radix_letter(q[-1])) != 0)
		slen = 1;

	if (pradix > sradix) {
		radix = pradix;
		r += plen;
	}
	else if (sradix > pradix) {
		radix = sradix;
		q -= slen;
	}
	else {
		/* Either decimal, or invalid -- if invalid, we'll trip up
		   further down. */
		radix = 10;
	}

	/*
	 * `checklimit' must be 2**64 / radix. We can't do that in
	 * 64-bit arithmetic, which we're (probably) using, so we
	 * cheat: since we know that all radices we use are even, we
	 * can divide 2**63 by radix/2 instead.
	 */
	checklimit = 0x8000000000000000ULL / (radix >> 1);

	/*
	 * Calculate the highest allowable value for the last digit of a
	 * 64-bit constant... in radix 10, it is 6, otherwise it is 0
	 */
	last = (radix == 10 ? 6 : 0);

	result = 0;
	while (*r && r < q) {
		if (*r != '_') {
			if (*r < '0' || (*r > '9' && *r < 'A')
				|| (digit = numvalue(*r)) >= radix) {
				*error = true;
				return 0;
			}
			if (result > checklimit ||
				(result == checklimit && digit >= last)) {
				warn = true;
			}

			result = radix * result + digit;
		}
		r++;
	}

	if (warn) {
		//--auto clean flag
		log_pass1 = 1;
		log_warnt = ERR_WARN_NOV;
		aasm_log(_LOG_WARN, "numeric constant %s does not fit in 64 bits", str);
	}
	return result * sign;
}

int64_t readstrnum(char* str, int length, bool* warn)
{
	int64_t charconst = 0;
	int i;

	*warn = false;

	str += length;
	if (globalbits == 64) {
		for (i = 0; i < length; i++) {
			if (charconst & 0xFF00000000000000ULL)
				*warn = true;
			charconst = (charconst << 8) + (uint8_t) * --str;
		}
	}
	else {
		for (i = 0; i < length; i++) {
			if (charconst & 0xFF000000UL)
				*warn = true;
			charconst = (charconst << 8) + (uint8_t) * --str;
		}
	}
	return charconst;
}


// ---- * ----
_ESYM_C
void* nasm_realloc(void* q, size_t size)
{
	void* p = q ? realloc(q, size) : malc(size);
	if (!p) {
		//--auto clean flag
		log_0file = 1;
		aasm_log(_LOG_FATAL, "out of memory");
	}
	return p;
}
