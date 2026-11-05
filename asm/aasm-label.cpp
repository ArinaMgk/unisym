// ASCII C TAB4 CRLF
// Docutitle: (Module) 
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

#include "../inc/c/stdinc.h"
#include "inc/aasm.h"
#include "inc/hashtbl.hpp"
#include "../inc/cpp/Format/Code.hpp"


/*
 * A local label is one that begins with exactly one period. Things
 * that begin with _two_ periods are NASM-specific things.
 *
 * If TASM compatibility is enabled, a local label can also begin with
 * @@, so @@local is a TASM compatible local label. Note that we only
 * check for the first @ symbol, although TASM requires both.
 */
#define islocal(l)                                              \
	(tasm_compatible_mode ?					\
	(((l)[0] == '.' || (l)[0] == '@') && (l)[1] != '.') :   \
	((l)[0] == '.' && (l)[1] != '.'))
#define islocalchar(c)						\
	(tasm_compatible_mode ?                                  \
	((c) == '.' || (c) == '@') :                            \
	((c) == '.'))

#define LABEL_BLOCK  128	/* no. of labels/block */
#define LBLK_SIZE    (LABEL_BLOCK*sizeof(union label))

#define END_LIST -3             /* don't clash with NO_SEG! */
#define END_BLOCK -2
#define BOGUS_VALUE -4

#define PERMTS_SIZE  16384	/* size of text blocks */
#if (PERMTS_SIZE < IDLEN_MAX)
#error "IPERMTS_SIZE must be greater than or equal to IDLEN_MAX"
#endif

/* values for label.defn.is_global */
#define DEFINED_BIT	1
#define GLOBAL_BIT	2
#define EXTERN_BIT	4
#define COMMON_BIT	8

#define NOT_DEFINED_YET 0
#define TYPE_MASK 3
#define LOCAL_SYMBOL (DEFINED_BIT)
#define GLOBAL_PLACEHOLDER (GLOBAL_BIT)
#define GLOBAL_SYMBOL (DEFINED_BIT|GLOBAL_BIT)

union label {                   /* actual label structures */
	struct {
		int32_t segment;
	int64_t offset;
		char *label, *special;
		int is_global, is_norm;
	} defn;
	struct {
		int32_t movingon;
		int64_t dummy;
		union label *next;
	} admin;
};

struct permts {                 /* permanent text storage */
	struct permts *next;        /* for the linked list */
	int size, usage;            /* size and used space in ... */
	char data[PERMTS_SIZE];     /* ... the data block itself */
};

_ESYM_C int64_t global_offset_changed;       /* defined in nasm.c */

static struct hash_table ltab;		/* labels hash table */
static union label *ldata;		/* all label data blocks */
static union label *lfree;		/* labels free block */
static struct permts *perm_head;        /* start of perm. text storage */
static struct permts *perm_tail;        /* end of perm. text storage */

static void init_block(union label *blk);
static char *perm_copy(const char *string);

static char *prevlabel;

static bool initialized = false;

char lprefix[PREFIX_MAX] = { 0 };
char lpostfix[PREFIX_MAX] = { 0 };

/*
 * Internal routine: finds the `union label' corresponding to the
 * given label name. Creates a new one, if it isn't found, and if
 * `create' is true.
 */
static union label *find_label(rostr label, int create)
{
	char *prev;
	int prevlen, len;
	union label *lptr, **lpp;
	char label_str[IDLEN_MAX];
	struct hash_insert ip;

	if (islocal(label)) {
		prev = prevlabel;
	prevlen = StrLength(prev);
	len = StrLength(label);
	if (prevlen+len >= IDLEN_MAX)
		return NULL;	/* Error... */
	MemCopyN(label_str, prev, prevlen);
	MemCopyN(label_str+prevlen, label, len+1);
	label = label_str;
	} else {
		prev = (char*)"";
	prevlen = 0;
	}

	lpp = (union label **) ltab.hash_find(label, &ip);
	//lpp = (union label **) hash_find(&ltab, label, &ip);
	lptr = lpp ? *lpp : NULL;

	if (lptr || !create)
	return lptr;

	/* Create a new label... */
	if (lfree->admin.movingon == END_BLOCK) {
	/*
	 * must allocate a new block
	 */
	lfree->admin.next =
		(union label *)malc(LBLK_SIZE);
	lfree = lfree->admin.next;
	init_block(lfree);
	}

	lfree->admin.movingon = BOGUS_VALUE;
	lfree->defn.label = perm_copy(label);
	lfree->defn.special = NULL;
	lfree->defn.is_global = NOT_DEFINED_YET;

	hash_add(&ip, lfree->defn.label, lfree);
	return lfree++;
}

bool Labelsys::Lookup(char* label, int32_t* segment, int64_t* offset)
{
	union label *lptr;

	if (!initialized)
		return false;

	lptr = find_label(label, 0);
	if (lptr && (lptr->defn.is_global & DEFINED_BIT)) {
		*segment = lptr->defn.segment;
		*offset = lptr->defn.offset;
		return true;
	} else
		return false;
}

bool Labelsys::IsExternal(char* label)
{
	union label *lptr;

	if (!initialized)
		return false;

	lptr = find_label(label, 0);
	return (lptr && (lptr->defn.is_global & EXTERN_BIT));
}

void Labelsys::Define(rostr label, int32_t segment, int64_t offset,
	char* special, bool is_norm, bool isextrn, outffmt* format)
{
	union label *lptr;
	int exi;

#ifdef DEBUG
#if DEBUG<3
	if (!strncmp(label, "debugdump", 9))
	#endif
		//--auto clean flag
		aasm_log(_LOG_DEBUG,
			"Labelsys::Define legacy (%s, %ld, %08lx, %s, %d, %d)",
			label, segment, offset, special, is_norm, isextrn);
#endif
	lptr = find_label(label, 1);
	if (lptr->defn.is_global & DEFINED_BIT) {
		//--auto clean flag
		aasm_log(_LOG_ERROR, "symbol `%s' redefined", label);
		return;
	}
	lptr->defn.is_global |= DEFINED_BIT;
	if (isextrn)
		lptr->defn.is_global |= EXTERN_BIT;

	if (!islocalchar(label[0]) && is_norm) {
	/* not local, but not special either */
		prevlabel = lptr->defn.label;
	}
	else if (islocal(label) && !*prevlabel) {
		//--auto clean flag
		aasm_log(_LOG_ERROR, "attempt to define a local label before any non-local labels");
	}

	lptr->defn.segment = segment;
	lptr->defn.offset = offset;
	lptr->defn.is_norm = (!islocalchar(label[0]) && is_norm);

	if (pass0 == 1 || (!is_norm && !isextrn && (segment > 0) && (segment & 1))) {
		exi = !!(lptr->defn.is_global & GLOBAL_BIT);
		if (exi) {
			char *xsymbol;
			int slen;
			slen = StrLength(lprefix);
			slen += StrLength(lptr->defn.label);
			slen += StrLength(lpostfix);
			slen++;             /* room for that null char */
			xsymbol = (char*)malc(slen);
			snprintf(xsymbol, slen, "%s%s%s", lprefix, lptr->defn.label,
					 lpostfix);

			format->symdef(xsymbol, segment, offset, exi,
						 special ? special : lptr->defn.special);
			format->current_dfmt->debug_deflabel(xsymbol, segment, offset,
											   exi,
											   special ? special : lptr->
											   defn.special);
/**	memf(xsymbol);  ! outobj.c stores the pointer; ouch!!! **/
		} else {
			if ((lptr->defn.is_global & (GLOBAL_BIT | EXTERN_BIT)) !=
				EXTERN_BIT) {
				format->symdef(lptr->defn.label, segment, offset, exi,
							 special ? special : lptr->defn.special);
				format->current_dfmt->debug_deflabel((char*)label, segment, offset, exi, special ? special : lptr->defn.special);
			}
		}
	}                           /* if (pass0 == 1) */
}

static void EmitCodeendSymbol(uni::CodeendInterface& codeend,
	char* name, int32_t segment, int64_t value, int binding,
	char* special)
{
	codeend.DefineSymbol(name, segment, value, binding, special);
	codeend.DefineDebugSymbol(name, segment, value, binding, special);
}

void Labelsys::Redefine(rostr label, int32_t segment, int64_t offset,
	char* special, bool is_norm, bool isextrn,
	uni::CodeendInterface& codeend)
{
	(void)is_norm;
	(void)isextrn;
	union label *lptr = find_label(label, 1);
	int exi;
	if (!lptr) {
		aasm_log(_LOG_PANIC, "can't find label `%s' on pass two", label);
		return;
	}

	if (!islocal(label) && !islocalchar(*label) && lptr->defn.is_norm)
		prevlabel = lptr->defn.label;
	if (lptr->defn.offset != offset) global_offset_changed++;
	lptr->defn.offset = offset;
	lptr->defn.segment = segment;

	if (pass0 != 1) return;
	exi = !!(lptr->defn.is_global & GLOBAL_BIT);
	if (exi) {
		int slen = StrLength(lprefix) + StrLength(lptr->defn.label) +
			StrLength(lpostfix) + 1;
		char* xsymbol = (char*)malc(slen);
		snprintf(xsymbol, slen, "%s%s%s", lprefix, lptr->defn.label,
			lpostfix);
		EmitCodeendSymbol(codeend, xsymbol, segment, offset, exi,
			special ? special : lptr->defn.special);
	}
	else if ((lptr->defn.is_global & (GLOBAL_BIT | EXTERN_BIT)) !=
		EXTERN_BIT) {
		EmitCodeendSymbol(codeend, lptr->defn.label, segment, offset, exi,
			special ? special : lptr->defn.special);
	}
}

void Labelsys::Define(rostr label, int32_t segment, int64_t offset,
	char* special, bool is_norm, bool isextrn,
	uni::CodeendInterface& codeend)
{
	union label *lptr = find_label(label, 1);
	int exi;
	if (lptr->defn.is_global & DEFINED_BIT) {
		aasm_log(_LOG_ERROR, "symbol `%s' redefined", label);
		return;
	}
	lptr->defn.is_global |= DEFINED_BIT;
	if (isextrn) lptr->defn.is_global |= EXTERN_BIT;

	if (!islocalchar(label[0]) && is_norm)
		prevlabel = lptr->defn.label;
	else if (islocal(label) && !*prevlabel)
		aasm_log(_LOG_ERROR,
			"attempt to define a local label before any non-local label");

	lptr->defn.segment = segment;
	lptr->defn.offset = offset;
	lptr->defn.is_norm = (!islocalchar(label[0]) && is_norm);

	if (!(pass0 == 1 || (!is_norm && !isextrn && (segment > 0) &&
		(segment & 1)))) return;
	exi = !!(lptr->defn.is_global & GLOBAL_BIT);
	if (exi) {
		int slen = StrLength(lprefix) + StrLength(lptr->defn.label) +
			StrLength(lpostfix) + 1;
		char* xsymbol = (char*)malc(slen);
		snprintf(xsymbol, slen, "%s%s%s", lprefix, lptr->defn.label,
			lpostfix);
		EmitCodeendSymbol(codeend, xsymbol, segment, offset, exi,
			special ? special : lptr->defn.special);
	}
	else if ((lptr->defn.is_global & (GLOBAL_BIT | EXTERN_BIT)) !=
		EXTERN_BIT) {
		EmitCodeendSymbol(codeend, lptr->defn.label, segment, offset, exi,
			special ? special : lptr->defn.special);
	}
}

void Labelsys::Common(char* label, int32_t segment, int32_t size,
	char* special, uni::CodeendInterface& codeend)
{
	union label *lptr = find_label(label, 1);
	if ((lptr->defn.is_global & DEFINED_BIT) &&
		(passn == 1 || !(lptr->defn.is_global & COMMON_BIT))) {
		aasm_log(_LOG_ERROR, "symbol `%s' redefined", label);
		return;
	}
	lptr->defn.is_global |= DEFINED_BIT | COMMON_BIT;
	if (!islocalchar(label[0])) prevlabel = lptr->defn.label;
	else {
		aasm_log(_LOG_ERROR,
			"attempt to define a local label as a common variable");
		return;
	}
	lptr->defn.segment = segment;
	lptr->defn.offset = 0;
	if (pass0 == 0) return;
	EmitCodeendSymbol(codeend, lptr->defn.label, segment, size, 2,
		special ? special : lptr->defn.special);
}

void Labelsys::DeclareGlobal(char* label, char* special)
{
	union label *lptr;

	if (islocal(label)) {
		//--auto clean flag
		aasm_log(_LOG_ERROR, "attempt to declare local symbol `%s' as global", label);
		return;
	}
	lptr = find_label(label, 1);
	switch (lptr->defn.is_global & TYPE_MASK) {
	case NOT_DEFINED_YET:
		lptr->defn.is_global = GLOBAL_PLACEHOLDER;
		lptr->defn.special = special ? perm_copy(special) : NULL;
		break;
	case GLOBAL_PLACEHOLDER:   /* already done: silently ignore */
	case GLOBAL_SYMBOL:
		break;
	case LOCAL_SYMBOL:
		if (!(lptr->defn.is_global & EXTERN_BIT)) {
			//--auto clean flag
			aasm_log(_LOG_WARN, "symbol `%s': GLOBAL directive after symbol definition is an experimental feature", label);
			lptr->defn.is_global = GLOBAL_SYMBOL;
		}
		break;
	}
}

void Labelsys::Initialize()
{
	ltab.hash_init(HASH_LARGE);
	//hash_init(&ltab, HASH_LARGE);

	ldata = lfree = (union label *)malc(LBLK_SIZE);
	init_block(lfree);

	perm_head =
		perm_tail = (struct permts *)malc(sizeof(struct permts));

	perm_head->next = NULL;
	perm_head->size = PERMTS_SIZE;
	perm_head->usage = 0;

	prevlabel = (char*)"";

	initialized = true;

}

void Labelsys::Cleanup()
{
	union label *lptr, *lhold;

	initialized = false;

	ltab.hash_free();

	lptr = lhold = ldata;
	while (lptr) {
	lptr = &lptr[LABEL_BLOCK-1];
	lptr = lptr->admin.next;
	memf(lhold);
	lhold = lptr;
	}

	while (perm_head) {
		perm_tail = perm_head;
		perm_head = perm_head->next;
		memf(perm_tail);
	}
}

static void init_block(union label *blk)
{
	int j;

	for (j = 0; j < LABEL_BLOCK - 1; j++)
		blk[j].admin.movingon = END_LIST;
	blk[LABEL_BLOCK - 1].admin.movingon = END_BLOCK;
	blk[LABEL_BLOCK - 1].admin.next = NULL;
}

static char *perm_copy(const char *string)
{
	char *p;
	int len = StrLength(string)+1;

	aasm_assert(len <= PERMTS_SIZE);

	if (perm_tail->size - perm_tail->usage < len) {
		perm_tail->next =
			(struct permts *)malc(sizeof(struct permts));
		perm_tail = perm_tail->next;
		perm_tail->next = NULL;
		perm_tail->size = PERMTS_SIZE;
		perm_tail->usage = 0;
	}
	p = perm_tail->data + perm_tail->usage;
	MemCopyN(p, string, len);
	perm_tail->usage += len;

	return p;
}

char* Labelsys::LocalScope(char* label)
{
	return islocal(label) ? prevlabel : (char*)"";
}

