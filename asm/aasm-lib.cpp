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
#include "inc/inst.h"


_ESYM_C uint64_t _crc64_tab_nasm[256];
_ESYM_C int globalbits = 0;

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

extern "C" void SegInit();
stdint SegAlloc() { // SegAlloc
	return (next_seg += 2) - 2;
}
void SegInit() { next_seg = 0; }


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
	//{UNCHK} Apply a specific string transform and return it in a aasm_malloc'd buffer, returning the length.  On error, returns (size_t)-1 and no buffer is allocated.
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


// ---- nulldbg ----

_ESYM_C{
void null_debug_init(struct outffmt* of, void* id, FILE* fp) {}
void null_debug_linenum(const char* filename, int32_t linenumber, int32_t segto) {}
void null_debug_deflabel(char* name, int32_t segment, int64_t offset, int is_global, char* special) {}
void null_debug_routine(const char* directive, const char* params) {}
void null_debug_typevalue(int32_t type) {}
void null_debug_output(int type, void* param) {}
void null_debug_cleanup(void) {}
}
_ESYM_C
struct dbgffmt null_debug_form = {
    "Null debug format",
    "null",
    null_debug_init,
    null_debug_linenum,
    null_debug_deflabel,
    null_debug_routine,
    null_debug_typevalue,
    null_debug_output,
    null_debug_cleanup
};

_ESYM_C
struct dbgffmt* null_debug_arr[2] = { &null_debug_form, NULL };



// ---- exprlib ----

/*
 * Return true if the argument is a simple scalar. (Or a far-
 * absolute, which counts.)
 */
int is_simple(expr * vect)
{
    while (vect->type && !vect->value)
        vect++;
    if (!vect->type)
        return 1;
    if (vect->type != EXPR_SIMPLE)
        return 0;
    do {
        vect++;
    } while (vect->type && !vect->value);
    if (vect->type && vect->type < EXPR_SEGBASE + SEG_ABS)
        return 0;
    return 1;
}

/*
 * Return true if the argument is a simple scalar, _NOT_ a far-
 * absolute.
 */
int is_really_simple(expr * vect)
{
    while (vect->type && !vect->value)
        vect++;
    if (!vect->type)
        return 1;
    if (vect->type != EXPR_SIMPLE)
        return 0;
    do {
        vect++;
    } while (vect->type && !vect->value);
    if (vect->type)
        return 0;
    return 1;
}

/*
 * Return true if the argument is relocatable (i.e. a simple
 * scalar, plus at most one segment-base, plus possibly a WRT).
 */
int is_reloc(expr * vect)
{
    while (vect->type && !vect->value)  /* skip initial value-0 terms */
        vect++;
    if (!vect->type)            /* trivially return true if nothing */
        return 1;               /* is present apart from value-0s */
    if (vect->type < EXPR_SIMPLE)       /* false if a register is present */
        return 0;
    if (vect->type == EXPR_SIMPLE) {    /* skip over a pure number term... */
        do {
            vect++;
        } while (vect->type && !vect->value);
        if (!vect->type)        /* ...returning true if that's all */
            return 1;
    }
    if (vect->type == EXPR_WRT) {       /* skip over a WRT term... */
        do {
            vect++;
        } while (vect->type && !vect->value);
        if (!vect->type)        /* ...returning true if that's all */
            return 1;
    }
    if (vect->value != 0 && vect->value != 1)
        return 0;               /* segment base multiplier non-unity */
    do {                        /* skip over _one_ seg-base term... */
        vect++;
    } while (vect->type && !vect->value);
    if (!vect->type)            /* ...returning true if that's all */
        return 1;
    return 0;                   /* And return false if there's more */
}

/*
 * Return true if the argument contains an `unknown' part.
 */
int is_unknown(expr * vect)
{
    while (vect->type && vect->type < EXPR_UNKNOWN)
        vect++;
    return (vect->type == EXPR_UNKNOWN);
}

/*
 * Return true if the argument contains nothing but an `unknown'
 * part.
 */
int is_just_unknown(expr * vect)
{
    while (vect->type && !vect->value)
        vect++;
    return (vect->type == EXPR_UNKNOWN);
}

/*
 * Return the scalar part of a relocatable vector. (Including
 * simple scalar vectors - those qualify as relocatable.)
 */
int64_t reloc_value(expr * vect)
{
    while (vect->type && !vect->value)
        vect++;
    if (!vect->type)
        return 0;
    if (vect->type == EXPR_SIMPLE)
        return vect->value;
    else
        return 0;
}

/*
 * Return the segment number of a relocatable vector, or NO_SEG for
 * simple scalars.
 */
int32_t reloc_seg(expr * vect)
{
    while (vect->type && (vect->type == EXPR_WRT || !vect->value))
        vect++;
    if (vect->type == EXPR_SIMPLE) {
        do {
            vect++;
        } while (vect->type && (vect->type == EXPR_WRT || !vect->value));
    }
    if (!vect->type)
        return NO_SEG;
    else
        return vect->type - EXPR_SEGBASE;
}

/*
 * Return the WRT segment number of a relocatable vector, or NO_SEG
 * if no WRT part is present.
 */
int32_t reloc_wrt(expr * vect)
{
    while (vect->type && vect->type < EXPR_WRT)
        vect++;
    if (vect->type == EXPR_WRT) {
        return vect->value;
    } else
        return NO_SEG;
}


// ---- stdscan ----

#ifndef NO_STDSCAN

/*
 * Standard scanner routine used by parser.c and some output
 * formats. It keeps a succession of temporary-storage strings in
 * stdscan_tempstorage, which can be cleared using stdscan_reset.
 */
static char **stdscan_tempstorage = NULL;
static int stdscan_tempsize = 0, stdscan_templen = 0;
#define STDSCAN_TEMP_DELTA 256

static void stdscan_pop(void)
{
	memf(stdscan_tempstorage[--stdscan_templen]);
}

void stdscan_reset(void)
{
	while (stdscan_templen > 0)
		stdscan_pop();
}

/*
 * Unimportant cleanup is done to avoid confusing people who are trying
 * to debug real memory leaks
 */
void stdscan_cleanup(void)
{
	stdscan_reset();
	memf(stdscan_tempstorage);
}

static char *stdscan_copy(char *p, int len)
{
	char *text;

	text = (char*)malc(len + 1);
	MemCopyN(text, p, len);
	text[len] = '\0';

	if (stdscan_templen >= stdscan_tempsize) {
		stdscan_tempsize += STDSCAN_TEMP_DELTA;
		stdscan_tempstorage = (char**)aasm_realloc(stdscan_tempstorage, stdscan_tempsize * sizeof(char *));
	}
	stdscan_tempstorage[stdscan_templen++] = text;

	return text;
}

char *stdscan_bufptr = NULL;
int stdscan(void* private_data, struct tokenval* tv)
{
	char ourcopy[MAX_KEYWORD + 1], * r, * s;

	(void)private_data;         /* Don't warn that this parameter is unused */

	while (ascii_isspace(*stdscan_bufptr))
		stdscan_bufptr++;
	if (!*stdscan_bufptr)
		return tv->t_type = (token_type)0;

	/* we have a token; either an id, a number or a char */
	if (isidstart(*stdscan_bufptr) ||
		(*stdscan_bufptr == '$' && isidstart(stdscan_bufptr[1]))) {
		/* now we've got an identifier */
		bool is_sym = false;

		if (*stdscan_bufptr == '$') {
			is_sym = true;
			stdscan_bufptr++;
		}

		r = stdscan_bufptr++;
		/* read the entire buffer to advance the buffer pointer but... */
		while (isidchar(*stdscan_bufptr))
			stdscan_bufptr++;

		/* ... copy only up to IDLEN_MAX-1 characters */
		tv->t_charptr = stdscan_copy(r, stdscan_bufptr - r < IDLEN_MAX ?
			stdscan_bufptr - r : IDLEN_MAX - 1);

		if (is_sym || stdscan_bufptr - r > MAX_KEYWORD)
			return tv->t_type = TOKEN_ID;       /* bypass all other checks */

		for (s = tv->t_charptr, r = ourcopy; *s; s++)
			*r++ = ascii_tolower(*s);
		*r = '\0';
		/* right, so we have an identifier sitting in temp storage. now,
		 * is it actually a register or instruction name, or what? */
		return aasm_token_hash(ourcopy, tv);
	}
	else if (*stdscan_bufptr == '$' && !isnumchar(stdscan_bufptr[1])) {
	 /*
	  * It's a $ sign with no following hex number; this must
	  * mean it's a Here token ($), evaluating to the current
	  * assembly location, or a Base token ($$), evaluating to
	  * the base of the current segment.
	  */
		stdscan_bufptr++;
		if (*stdscan_bufptr == '$') {
			stdscan_bufptr++;
			return tv->t_type = TOKEN_BASE;
		}
		return tv->t_type = TOKEN_HERE;
	}
	else if (isnumstart(*stdscan_bufptr)) {   /* now we've got a number */
		bool rn_error = 0;
		bool is_hex = false;
		bool is_float = false;
		bool has_e = false;
		char c;

		r = stdscan_bufptr;

		if (*stdscan_bufptr == '$') {
			stdscan_bufptr++;
			is_hex = true;
		}

		for (;;) {
			c = *stdscan_bufptr++;

			if (!is_hex && (c == 'e' || c == 'E')) {
				has_e = true;
				if (*stdscan_bufptr == '+' || *stdscan_bufptr == '-') {
					/* e can only be followed by +/- if it is either a
					   prefixed hex number or a floating-point number */
					is_float = true;
					stdscan_bufptr++;
				}
			}
			else if (c == 'H' || c == 'h' || c == 'X' || c == 'x') {
				is_hex = true;
			}
			else if (c == 'P' || c == 'p') {
				is_float = true;
				if (*stdscan_bufptr == '+' || *stdscan_bufptr == '-')
					stdscan_bufptr++;
			}
			else if (isnumchar(c) || c == '_')
				; /* just advance */
			else if (c == '.')
				is_float = true;
			else
				break;
		}
		stdscan_bufptr--;	/* Point to first character beyond number */

		if (has_e && !is_hex) {
			/* 1e13 is floating-point, but 1e13h is not */
			is_float = true;
		}

		if (is_float) {
			tv->t_charptr = stdscan_copy(r, stdscan_bufptr - r);
			return tv->t_type = TOKEN_FLOAT;
		}
		else {
			r = stdscan_copy(r, stdscan_bufptr - r);
			tv->t_integer = readnum(r, &rn_error);
			stdscan_pop();
			if (rn_error) {
			/* some malformation occurred */
				return tv->t_type = TOKEN_ERRNUM;
			}
			tv->t_charptr = NULL;
			return tv->t_type = TOKEN_NUM;
		}
	}
	else if (*stdscan_bufptr == '\'' || *stdscan_bufptr == '"' ||
		*stdscan_bufptr == '`') {
 /* a quoted string */
		char start_quote = *stdscan_bufptr;
		tv->t_charptr = stdscan_bufptr;
		tv->t_inttwo = aasm_unquote(tv->t_charptr, &stdscan_bufptr);
		if (*stdscan_bufptr != start_quote)
			return tv->t_type = TOKEN_ERRSTR;
		stdscan_bufptr++;	/* Skip final quote */
		return tv->t_type = TOKEN_STR;
	}
	else if (*stdscan_bufptr == ';') {
	 /* a comment has happened - stay */
		return tv->t_type = (token_type)0;
	}
	else if (stdscan_bufptr[0] == '>' && stdscan_bufptr[1] == '>') {
		stdscan_bufptr += 2;
		return tv->t_type = TOKEN_SHR;
	}
	else if (stdscan_bufptr[0] == '<' && stdscan_bufptr[1] == '<') {
		stdscan_bufptr += 2;
		return tv->t_type = TOKEN_SHL;
	}
	else if (stdscan_bufptr[0] == '/' && stdscan_bufptr[1] == '/') {
		stdscan_bufptr += 2;
		return tv->t_type = TOKEN_SDIV;
	}
	else if (stdscan_bufptr[0] == '%' && stdscan_bufptr[1] == '%') {
		stdscan_bufptr += 2;
		return tv->t_type = TOKEN_SMOD;
	}
	else if (stdscan_bufptr[0] == '=' && stdscan_bufptr[1] == '=') {
		stdscan_bufptr += 2;
		return tv->t_type = TOKEN_EQ;
	}
	else if (stdscan_bufptr[0] == '<' && stdscan_bufptr[1] == '>') {
		stdscan_bufptr += 2;
		return tv->t_type = TOKEN_NE;
	}
	else if (stdscan_bufptr[0] == '!' && stdscan_bufptr[1] == '=') {
		stdscan_bufptr += 2;
		return tv->t_type = TOKEN_NE;
	}
	else if (stdscan_bufptr[0] == '<' && stdscan_bufptr[1] == '=') {
		stdscan_bufptr += 2;
		return tv->t_type = TOKEN_LE;
	}
	else if (stdscan_bufptr[0] == '>' && stdscan_bufptr[1] == '=') {
		stdscan_bufptr += 2;
		return tv->t_type = TOKEN_GE;
	}
	else if (stdscan_bufptr[0] == '&' && stdscan_bufptr[1] == '&') {
		stdscan_bufptr += 2;
		return tv->t_type = TOKEN_DBL_AND;
	}
	else if (stdscan_bufptr[0] == '^' && stdscan_bufptr[1] == '^') {
		stdscan_bufptr += 2;
		return tv->t_type = TOKEN_DBL_XOR;
	}
	else if (stdscan_bufptr[0] == '|' && stdscan_bufptr[1] == '|') {
		stdscan_bufptr += 2;
		return tv->t_type = TOKEN_DBL_OR;
	}
	else                      /* just an ordinary char */
		return tv->t_type = (token_type)(uint8_t)(*stdscan_bufptr++);
}

#endif

// ---- * ----

/* Uninitialized -> all zero by C spec */
const uint8_t zero_buffer[ZERO_BUF_SIZE] = {0};

_ESYM_C
void* aasm_realloc(void* q, size_t size)
{
	void* p = q ? realloc(q, size) : malc(size);
	if (!p) {
		//--auto clean flag
		log_0file = 1;
		aasm_log(_LOG_FATAL, "out of memory");
	}
	return p;
}

/*
 * Common list of prefix names
 */
static const char *prefix_names[] = {
	"a16", "a32", "a64", "asp", "lock", "o16", "o32", "o64", "osp",
	"rep", "repe", "repne", "repnz", "repz", "times", "wait"
};
const char *prefix_name(int token)
{
	unsigned int prefix = token-PREFIX_ENUM_START;
	if (prefix > numsof(prefix_names))
	return NULL;

	return prefix_names[prefix];
}

_ESYM_C void exit(int);
void assert_failed(const char *file, int line, const char *msg)
{
	//--auto clean flag
	aasm_log(_LOG_FATAL, "assertion %s failed at %s:%d", msg, file, line);
	exit(1);
}

char* aasm_strsep(char** stringp, const char* delim)
{
	char* s = *stringp;
	char* e;
	if (!s) return NULL;
	e = (char*)StrIndexChars(s, delim);
	if (e)
		*e++ = '\0';
	*stringp = e;
	return s;
}

void fwriteint16_t(uint16_t data, FILE * fp)
{
	MemReverseL(data);
	fwrite(&data, 1, 2, fp);
}

void fwriteint32_t(uint32_t data, FILE * fp)
{
	MemReverseL(data);
	fwrite(&data, 1, 4, fp);
}

void fwriteint64_t(uint64_t data, FILE * fp)
{
	MemReverseL(data);
	fwrite(&data, 1, 8, fp);
}

void fwriteaddr(uint64_t data, int size, FILE * fp)
{
	MemReverseL(data);
	fwrite(&data, 1, size, fp);
}

size_t fwritezero(size_t bytes, FILE *fp)
{
	size_t count = 0;
	size_t blksize;
	size_t rv;
	while (bytes) {
	blksize = (bytes < ZERO_BUF_SIZE) ? bytes : ZERO_BUF_SIZE;
	rv = fwrite(zero_buffer, 1, blksize, fp);
	if (!rv)
		break;
	count += rv;
	bytes -= rv;
	}
	return count;
}

void standard_extension(char* inname, char* outname, rostr extension)
{
	char* p, * q;

	if (*outname)               /* file name already exists, */
		return;                 /* so do nothing */
	q = inname;
	p = outname;
	while (*q)
		*p++ = *q++;            /* copy, and find end of string */
	*p = '\0';                  /* terminate it */
	while (p > outname && *--p != '.');        /* find final period (or whatever) */
	if (*p != '.')
		while (*p)
			p++;                /* go back to end if none found */
	if (!StrCompare(p, extension)) {        /* is the extension already there? */
		//--auto clean flag
		log_0file = 1;
		if (*extension)
			aasm_log(_LOG_WARN, "file name already ends in `%s': output will be in `nasm.out'", extension);
		else
			aasm_log(_LOG_WARN, "file name already has no extension: output will be in `nasm.out'");
		StrCopy(outname, "nasm.out");
	}
	else
		StrCopy(p, extension);
}

int bsi(const char *string, const char **array, int size)// Binary search
{
	int i = -1, j = size;       /* always, i < index < j */
	while (j - i >= 2) {
		int k = (i + j) / 2;
		int l = StrCompare(string, array[k]);
		if (l < 0)              /* it's in the first half */
			j = k;
		else if (l > 0)         /* it's in the second half */
			i = k;
		else                    /* we've got it :) */
			return k;
	}
	return -1;                  /* we haven't got it :( */
}
int bsii(const char *string, const char **array, int size)
{
	int i = -1, j = size;       /* always, i < index < j */
	while (j - i >= 2) {
		int k = (i + j) / 2;
		int l = StrCompareInsensitive(string, array[k]);
		if (l < 0)              /* it's in the first half */
			j = k;
		else if (l > 0)         /* it's in the second half */
			i = k;
		else                    /* we've got it :) */
			return k;
	}
	return -1;                  /* we haven't got it :( */
}

static char *file_name = NULL;
static int32_t line_number = 0;

char *src_set_fname(char *newname)
{
	char *oldname = file_name;
	file_name = newname;
	return oldname;
}

int32_t src_set_linnum(int32_t newline)
{
	int32_t oldline = line_number;
	line_number = newline;
	return oldline;
}

int32_t src_get_linnum(void)
{
	return line_number;
}

int src_get(int32_t *xline, char **xname)
{
	if (!file_name || !*xname || StrCompare(*xname, file_name)) {
		memf(*xname);
		*xname = file_name ? StrHeap(file_name) : NULL;
		*xline = line_number;
		return -2;
	}
	if (*xline != line_number) {
		int32_t tmp = line_number - *xline;
		*xline = line_number;
		return tmp;
	}
	return 0;
}


