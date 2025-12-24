// ASCII C TAB4 CRLF
// Docutitle: (Format.Executable) ELF64
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Bit-32mode Non-Dependence
// Copyright: UNISYM, under Apache License 2.0; Dosconio Mecocoa, BSD 3-Clause License
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

// ---- ---- AASM ---- ---- //
#define ELFBIT 64

#include "../../../../inc/c/strbuff.h"
#include "../../../../inc/c/strpage.h"
#include "../../../../inc/c/format/ELF.h"
#include "../../../../inc/c/format/DWARF.h"
#include "../../../../asm/inc/aasm.h"
#include "../../../../asm/inc/rbtree.hpp"

static const struct elf_known_section elf_known_sections[] = {
	{ ".text",    SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR,     16 },
	{ ".rodata",  SHT_PROGBITS, SHF_ALLOC,                    4 },
	{ ".lrodata", SHT_PROGBITS, SHF_ALLOC,                    4 },
	{ ".data",    SHT_PROGBITS, SHF_ALLOC | SHF_WRITE,          4 },
	{ ".ldata",   SHT_PROGBITS, SHF_ALLOC | SHF_WRITE,          4 },
	{ ".bss",     SHT_NOBITS,   SHF_ALLOC | SHF_WRITE,          4 },
	{ ".lbss",    SHT_NOBITS,   SHF_ALLOC | SHF_WRITE,          4 },
	{ ".tdata",   SHT_PROGBITS, SHF_ALLOC | SHF_WRITE | SHF_TLS,  4 },
	{ ".tbss",    SHT_NOBITS,   SHF_ALLOC | SHF_WRITE | SHF_TLS,  4 },
	{ ".comment", SHT_PROGBITS, 0,                            1 },
	{ NULL,       SHT_PROGBITS, SHF_ALLOC,                    1 } /* default */
};

#define SOC(ln,aa) ln - line_base + (line_range * aa) + opcode_base

struct Reloc {
	struct Reloc *next;
	int64_t address;            /* relative to _start_ of section */
	int64_t symbol;             /* symbol index */
	int64_t offset;		/* symbol addend */
	int type;			/* type of relocation */
};

struct Symbol {
	struct rbtree symv;	        /* symbol value and rbtree of globals */
	int32_t strpos;             /* string table position of name */
	int32_t section;            /* section ID of the symbol */
	int type;			/* symbol type */
	int other;                  /* symbol visibility */
	int32_t size;               /* size of symbol */
	int32_t globnum;            /* symbol table offset if global */
	struct Symbol *nextfwd;     /* list of unresolved-size symbols */
	char *name;			/* used temporarily if in above list */
};

struct Section {
	Strbuff *data;
	uint64_t len, size;
	uint32_t nrelocs;
	int32_t index;	       	/* index into sects array */
	int type;             	/* SHT_PROGBITS or SHT_NOBITS */
	uint64_t align;            	/* alignment: power of two */
	uint64_t flags;            	/* section flags */
	char *name;
	Strbuff *rel;
	uint64_t rellen;
	struct Reloc *head, **tail;
	struct rbtree *gsyms;       /* global symbols in section */
};

#define SECT_DELTA 32
static struct Section **sects;
static int nsects, sectlen;

#define SHSTR_DELTA 256
static char *shstrtab;
static int shstrtablen, shstrtabsize;

static Strbuff *syms;
static uint32_t nlocals, nglobs, ndebugs;

static int32_t def_seg;

static Strpage *bsym;

static Strbuff *strs;
static uint32_t strslen;

static FILE *elffp;

static evalfunc evaluate;

static struct Symbol *fwds;

static char elf_module[FILENAME_MAX];

static uint8_t elf_osabi = 0;	/* Default OSABI = 0 (System V or Linux) */
static uint8_t elf_abiver = 0;	/* Current ABI version */

_ESYM_C struct outffmt of_elf64;

static struct ELF_SECTDATA {
	void *data;
	int64_t len;
	bool is_saa;
} *elf_sects;
static int elf_nsect, nsections;
static int64_t elf_foffs;

static void elf_write(void);
static void elf_sect_write(struct Section *, const void *, size_t);
static void elf_sect_writeaddr(struct Section *, int64_t, size_t);
static void elf_section_header(int, int, uint64_t, void *, bool, uint64_t, int, int,
							   int, int);
static void elf_write_sections(void);
static Strbuff *elf_build_symtab(int32_t *, int32_t *);
static Strbuff *elf_build_reltab(uint64_t *, struct Reloc *);
static void add_sectname(rostr firsthalf, rostr secondhalf);

/* type values for stabs debugging sections */
#define N_SO 0x64               /* ID for main source file */
#define N_SOL 0x84              /* ID for sub-source file */
#define N_BINCL 0x82            /* not currently used */
#define N_EINCL 0xA2            /* not currently used */
#define N_SLINE 0x44

struct stabentry {
	uint32_t n_strx;
	uint8_t n_type;
	uint8_t n_other;
	uint16_t n_desc;
	uint32_t n_value;
};

struct erel {
	int offset, info;
};

struct symlininfo {
	int offset;
	int section;                /* index into sects[] */
	int segto;			/* internal section number */
	char *name;                 /* shallow-copied pointer of section name */
};

struct linelist {
	struct symlininfo info;
	int line;
	char *filename;
	struct linelist *next;
	struct linelist *last;
};

struct sectlist {
	Strbuff *psaa;
	int section;
	int line;
	int offset;
	int file;
	struct sectlist *next;
	struct sectlist *last;
};

/* common debug variables */
static int currentline = 1;
static int debug_immcall = 0;

/* stabs debug variables */
static struct linelist *stabslines = 0;
static int numlinestabs = 0;
static char *stabs_filename = 0;
static int symtabsection;
static uint8_t *stabbuf = 0, *stabstrbuf = 0, *stabrelbuf = 0;
static int stablen, stabStrLength, stabrellen;

/* dwarf debug variables */
static struct linelist *dwarf_flist = 0, *dwarf_clist = 0, *dwarf_elist = 0;
static struct sectlist *dwarf_fsect = 0, *dwarf_csect = 0, *dwarf_esect = 0;
static int dwarf_numfiles = 0, dwarf_nsections;
static uint8_t *arangesbuf = 0, *arangesrelbuf = 0, *pubnamesbuf = 0, *infobuf = 0,  *inforelbuf = 0,
			   *abbrevbuf = 0, *linebuf = 0, *linerelbuf = 0, *framebuf = 0, *locbuf = 0;
static int8_t line_base = -5, line_range = 14, opcode_base = 13;
static int arangeslen, arangesrellen, pubnameslen, infolen, inforellen,
		   abbrevlen, linelen, linerellen, framelen, loclen;
static int64_t dwarf_infosym, dwarf_abbrevsym, dwarf_linesym;

static struct Symbol *lastsym;

/* common debugging routines */
static void debug64_typevalue(int32_t);
static void debug64_deflabel(char *, int32_t, int64_t, int, char *);
static void debug64_directive(const char *, const char *);

/* stabs debugging routines */
static void stabs64_linenum(const char *filename, int32_t linenumber, int32_t);
static void stabs64_output(int, void *);
static void stabs64_generate(void);
static void stabs64_cleanup(void);

/* dwarf debugging routines */
static void dwarf64_init(struct outffmt *, void *, FILE *);
static void dwarf64_linenum(const char *filename, int32_t linenumber, int32_t);
static void dwarf64_output(int, void *);
static void dwarf64_generate(void);
static void dwarf64_cleanup(void);
static void dwarf64_findfile(const char *);
static void dwarf64_findsect(const int);

static struct dbgffmt df_dwarf = {
	"ELF64 (x86-64) dwarf debug format for Linux/Unix",
	"dwarf",
	dwarf64_init,
	dwarf64_linenum,
	debug64_deflabel,
	debug64_directive,
	debug64_typevalue,
	dwarf64_output,
	dwarf64_cleanup
};
static struct dbgffmt df_stabs = {
	"ELF64 (x86-64) stabs debug format for Linux/Unix",
	"stabs",
	null_debug_init,
	stabs64_linenum,
	debug64_deflabel,
	debug64_directive,
	debug64_typevalue,
	stabs64_output,
	stabs64_cleanup
};

struct dbgffmt* elf64_debugs_arr[3] = { &df_dwarf, &df_stabs, NULL };
_ESYM_C macros_t elf_stdmac[];
static void elf_init(FILE* fp, ldfunc ldef, evalfunc eval);
static int elf_set_info(geninfo type, char** val);
static void elf_out(int32_t segto, const void* data, out_type type, uint64_t size, int32_t segment, int32_t wrt);
static void elf_deflabel(char* name, int32_t segment, int64_t offset, int is_global, char* special);
static int32_t elf_section_names(rostr name, int pass, int* bits);
static int32_t elf_segbase(int32_t segment);
static int elf_directive(char* directive, char* value, int pass);
static void elf_filename(char* inname, char* outname);
static void elf_cleanup(int debuginfo);

struct outffmt of_elf64 = {
	"ELF64 (x86_64) object files (e.g. Linux)",
	"elf64",
	0,
	elf64_debugs_arr,
	&df_stabs,
	elf_stdmac,
	elf_init,
	elf_set_info,
	elf_out,
	elf_deflabel,
	elf_section_names,
	elf_segbase,
	elf_directive,
	elf_filename,
	elf_cleanup
};

/*
 * Special section numbers which are used to define ELF special
 * symbols, which can be used with WRT to provide PIC relocation
 * types.
 */
static int32_t elf_gotpc_sect, elf_gotoff_sect;
static int32_t elf_got_sect, elf_plt_sect;
static int32_t elf_sym_sect;
static int32_t elf_gottpoff_sect;

static void elf_init(FILE * fp, ldfunc ldef, evalfunc eval)
{
	maxbits = 64;
	elffp = fp;
	evaluate = eval;
	(void)ldef;                 /* placate optimisers */
	sects = NULL;
	nsects = sectlen = 0;
	syms = StrbuffNew((int32_t)sizeof(struct Symbol));
	nlocals = nglobs = ndebugs = 0;
	bsym = StrpageNew();
	strs = StrbuffNew(1L);
	StrbuffSend(strs, (pureptr_t)"\0", 1L);
	StrbuffSend(strs, (pureptr_t)elf_module, (int32_t)(StrLength(elf_module) + 1));
	strslen = 2 + StrLength(elf_module);
	shstrtab = NULL;
	shstrtablen = shstrtabsize = 0;;
	add_sectname("", "");

	fwds = NULL;

	elf_gotpc_sect = SegAlloc();
	ldef("..gotpc", elf_gotpc_sect + 1, 0L, NULL, false, false, &of_elf64
		 );
	elf_gotoff_sect = SegAlloc();
	ldef("..gotoff", elf_gotoff_sect + 1, 0L, NULL, false, false, &of_elf64
		 );
	elf_got_sect = SegAlloc();
	ldef("..got", elf_got_sect + 1, 0L, NULL, false, false, &of_elf64
		 );
	elf_plt_sect = SegAlloc();
	ldef("..plt", elf_plt_sect + 1, 0L, NULL, false, false, &of_elf64
		 );
	elf_sym_sect = SegAlloc();
	ldef("..sym", elf_sym_sect + 1, 0L, NULL, false, false, &of_elf64
		 );
	elf_gottpoff_sect = SegAlloc();
	ldef("..gottpoff", elf_gottpoff_sect + 1, 0L, NULL, false, false, &of_elf64
		 );

	def_seg = SegAlloc();

}

static void elf_cleanup(int debuginfo)
{
	struct Reloc *r;
	int i;

	(void)debuginfo;

	elf_write();
	fclose(elffp);
	for (i = 0; i < nsects; i++) {
		if (sects[i]->type != SHT_NOBITS)
			StrbuffFree(sects[i]->data);
		if (sects[i]->head)
			StrbuffFree(sects[i]->rel);
		while (sects[i]->head) {
			r = sects[i]->head;
			sects[i]->head = sects[i]->head->next;
			memf(r);
		}
	}
	memf(sects);
	StrbuffFree(syms);
	StrpageFree(bsym);
	StrbuffFree(strs);
	if (of_elf64.current_dfmt) {
		of_elf64.current_dfmt->cleanup();
	}
}
/* add entry to the elf .shstrtab section */
static void add_sectname(rostr firsthalf, rostr secondhalf)
{
	int len = StrLength(firsthalf) + StrLength(secondhalf);
	while (shstrtablen + len + 1 > shstrtabsize)
		shstrtab = (char*)realloc(shstrtab, (shstrtabsize += SHSTR_DELTA));
	StrCopy(shstrtab + shstrtablen, firsthalf);
	StrAppend(shstrtab + shstrtablen, secondhalf);
	shstrtablen += len + 1;
}

static int elf_make_section(rostr name, int type, int flags, int align)
{
	struct Section *s;

	s = (Section*)malc(sizeof(*s));

	if (type != SHT_NOBITS)
		s->data = StrbuffNew(1L);
	s->head = NULL;
	s->tail = &s->head;
	s->len = s->size = 0;
	s->nrelocs = 0;
	if (!StrCompare(name, ".text"))
		s->index = def_seg;
	else
		s->index = SegAlloc();
	add_sectname("", name);
	s->name = (char*)malc(1 + StrLength(name));
	StrCopy(s->name, name);
	s->type = type;
	s->flags = flags;
	s->align = align;
	s->gsyms = NULL;

	if (nsects >= sectlen)
		sects = (Section**)realloc(sects, (sectlen += SECT_DELTA) * sizeof(*sects));
	sects[nsects++] = s;

	return nsects - 1;
}

static int32_t elf_section_names(rostr name, int pass, int *bits)
{
	char *p;
	uint32_t flags, flags_and, flags_or;
	uint64_t align;
	int type, i;

	/*
	 * Default is 64 bits.
	 */
	if (!name) {
		*bits = 64;
		return def_seg;
	}

	p = (char*)name;
	while (*p && !ascii_isspace(*p))
		p++;
	if (*p)
		*p++ = '\0';
	flags_and = flags_or = type = align = 0;

	while (*p && ascii_isspace(*p))
		p++;
	while (*p) {
		char *q = p;
		while (*p && !ascii_isspace(*p))
			p++;
		if (*p)
			*p++ = '\0';
		while (*p && ascii_isspace(*p))
			p++;

		if (!StrCompareNInsensitive(q, "align=", 6)) {
			align = atoins(q + 6);
			if (align == 0)
				align = 1;
			if ((align - 1) & align) {  /* means it's not a power of two */
				aasm_log(_LOG_ERROR, "section alignment %d is not"
					  " a power of two", align);
				align = 1;
			}
		} else if (!StrCompareInsensitive(q, "alloc")) {
			flags_and |= SHF_ALLOC;
			flags_or |= SHF_ALLOC;
		} else if (!StrCompareInsensitive(q, "noalloc")) {
			flags_and |= SHF_ALLOC;
			flags_or &= ~SHF_ALLOC;
		} else if (!StrCompareInsensitive(q, "exec")) {
			flags_and |= SHF_EXECINSTR;
			flags_or |= SHF_EXECINSTR;
		} else if (!StrCompareInsensitive(q, "noexec")) {
			flags_and |= SHF_EXECINSTR;
			flags_or &= ~SHF_EXECINSTR;
		} else if (!StrCompareInsensitive(q, "write")) {
			flags_and |= SHF_WRITE;
			flags_or |= SHF_WRITE;
		} else if (!StrCompareInsensitive(q, "tls")) {
			flags_and |= SHF_TLS;
			flags_or |= SHF_TLS;
		} else if (!StrCompareInsensitive(q, "nowrite")) {
			flags_and |= SHF_WRITE;
			flags_or &= ~SHF_WRITE;
		} else if (!StrCompareInsensitive(q, "progbits")) {
			type = SHT_PROGBITS;
		} else if (!StrCompareInsensitive(q, "nobits")) {
			type = SHT_NOBITS;
		} else if (pass == 1) {
		aasm_log(_LOG_WARN, "Unknown section attribute '%s' ignored on"
				  " declaration of section `%s'", q, name);
	}
	}

	if (!StrCompare(name, ".shstrtab") ||
		!StrCompare(name, ".symtab") ||
		!StrCompare(name, ".strtab")) {
		aasm_log(_LOG_ERROR, "attempt to redefine reserved section"
			  "name `%s'", name);
		return NO_SEG;
	}

	for (i = 0; i < nsects; i++)
		if (!StrCompare(name, sects[i]->name))
			break;
	if (i == nsects) {
	const struct elf_known_section *ks = elf_known_sections;

	while (ks->name) {
		if (!StrCompare(name, ks->name))
		break;
		ks++;
	}

	type = type ? type : ks->type;
	align = align ? align : ks->align;
	flags = (ks->flags & ~flags_and) | flags_or;

	i = elf_make_section(name, type, flags, align);
	} else if (pass == 1) {
		  if ((type && sects[i]->type != type)
			 || (align && sects[i]->align != align)
			 || (flags_and && ((sects[i]->flags & flags_and) != flags_or)))
			aasm_log(_LOG_WARN, "incompatible section attributes ignored on"
				  " redeclaration of section `%s'", name);
	}

	return sects[i]->index;
}

static void elf_deflabel(char *name, int32_t segment, int64_t offset,
						 int is_global, char *special)
{
	int pos = strslen;
	struct Symbol *sym;
	bool special_used = false;

#if defined(DEBUG) && DEBUG>2
	fprintf(stderr,
			" elf_deflabel: %s, seg=%x, off=%x, is_global=%d, %s\n",
			name, segment, offset, is_global, special);
#endif
	if (name[0] == '.' && name[1] == '.' && name[2] != '@') {
		/*
		 * This is a NASM special symbol. We never allow it into
		 * the ELF symbol table, even if it's a valid one. If it
		 * _isn't_ a valid one, we should barf immediately.
		 */
		if (StrCompare(name, "..gotpc") && StrCompare(name, "..gotoff") &&
			StrCompare(name, "..got") && StrCompare(name, "..plt") &&
			StrCompare(name, "..sym") && StrCompare(name, "..gottpoff"))
			aasm_log(_LOG_ERROR, "unrecognised special symbol `%s'", name);
		return;
	}

	if (is_global == 3) {
		struct Symbol **s;
		/*
		 * Fix up a forward-reference symbol size from the first
		 * pass.
		 */
		for (s = &fwds; *s; s = &(*s)->nextfwd)
			if (!StrCompare((*s)->name, name)) {
				struct tokenval tokval;
				expr *e;
				char *p = special;

				while (*p && !ascii_isspace(*p))
					p++;
				while (*p && ascii_isspace(*p))
					p++;
				stdscan_reset();
				stdscan_bufptr = p;
				tokval.t_type = TOKEN_INVALID;
				e = evaluate(stdscan, NULL, &tokval, NULL, 1, NULL);
				if (e) {
					if (!is_simple(e))
						aasm_log(_LOG_ERROR, "cannot use relocatable"
							  " expression as symbol size");
					else
						(*s)->size = reloc_value(e);
				}

				/*
				 * Remove it from the list of unresolved sizes.
				 */
				memf((*s)->name);
				*s = (*s)->nextfwd;
				return;
			}
		return;                 /* it wasn't an important one */
	}

	StrbuffSend(strs, name, (int32_t)(1 + StrLength(name)));
	strslen += 1 + StrLength(name);

	lastsym = sym = (Symbol*)StrbuffEalloc(syms);

	MemSet(&sym->symv, 0, sizeof(struct rbtree));

	sym->strpos = pos;
	sym->type = is_global ? SYM_GLOBAL : 0;
	sym->other = STV_DEFAULT;
	sym->size = 0;
	if (segment == NO_SEG)
		sym->section = SHN_ABS;
	else {
		int i;
		sym->section = SHN_UNDEF;
		if (nsects == 0 && segment == def_seg) {
			int tempint;
			if (segment != elf_section_names(".text", 2, &tempint))
				aasm_log(_LOG_PANIC,
					  "strange segment conditions in ELF driver");
			sym->section = nsects;
		} else {
			for (i = 0; i < nsects; i++)
				if (segment == sects[i]->index) {
					sym->section = i + 1;
					break;
				}
		}
	}

	if (is_global == 2) {
		sym->size = offset;
		sym->symv.key = 0;
		sym->section = SHN_COMMON;
		/*
		 * We have a common variable. Check the special text to see
		 * if it's a valid number and power of two; if so, store it
		 * as the alignment for the common variable.
		 */
		if (special) {
			bool err = 0;
			sym->symv.key = readnum(special, &err);
			if (err)
				aasm_log(_LOG_ERROR, "alignment constraint `%s' is not a"
					  " valid number", special);
			else if ((sym->symv.key | (sym->symv.key - 1))
			 != 2 * sym->symv.key - 1)
				aasm_log(_LOG_ERROR, "alignment constraint `%s' is not a"
					  " power of two", special);
		}
		special_used = true;
	} else
		sym->symv.key = (sym->section == SHN_UNDEF ? 0 : offset);

	if (sym->type == SYM_GLOBAL) {
		/*
		 * If sym->section == SHN_ABS, then the first line of the
		 * else section would cause a core dump, because its a reference
		 * beyond the end of the section array.
		 * This behaviour is exhibited by this code:
		 *     GLOBAL crash_nasm
		 *     crash_nasm equ 0
		 * To avoid such a crash, such requests are silently discarded.
		 * This may not be the best solution.
		 */
		if (sym->section == SHN_UNDEF || sym->section == SHN_COMMON) {
			bsym = StrpageSet(bsym, segment, nglobs);
		} else if (sym->section != SHN_ABS) {
			/*
			 * This is a global symbol; so we must add it to the rbtree
			 * of global symbols in its section.
			 *
			 * In addition, we check the special text for symbol
			 * type and size information.
			 */
		sects[sym->section-1]->gsyms = sects[sym->section-1]->gsyms->rb_insert(&sym->symv);

			if (special) {
				int n = StrSpanExclude(special, " \t");

				if (!StrCompareNInsensitive(special, "function", n))
					sym->type |= STT_FUNC;
				else if (!StrCompareNInsensitive(special, "data", n) ||
						 !StrCompareNInsensitive(special, "object", n))
					sym->type |= STT_OBJECT;
				else if (!StrCompareNInsensitive(special, "notype", n))
					sym->type |= STT_NOTYPE;
				else
					aasm_log(_LOG_ERROR, "unrecognised symbol type `%.*s'",
						  n, special);
				special += n;

				while (ascii_isspace(*special))
					++special;
				if (*special) {
					n = StrSpanExclude(special, " \t");
					if (!StrCompareNInsensitive(special, "default", n))
						sym->other = STV_DEFAULT;
					else if (!StrCompareNInsensitive(special, "internal", n))
						sym->other = STV_INTERNAL;
					else if (!StrCompareNInsensitive(special, "hidden", n))
						sym->other = STV_HIDDEN;
					else if (!StrCompareNInsensitive(special, "protected", n))
						sym->other = STV_PROTECTED;
					else
						n = 0;
					special += n;
				}

				if (*special) {
					struct tokenval tokval;
					expr *e;
			int fwd = 0;
					char *saveme = stdscan_bufptr;      /* bugfix? fbk 8/10/00 */

					while (special[n] && ascii_isspace(special[n]))
						n++;
					/*
					 * We have a size expression; attempt to
					 * evaluate it.
					 */
					stdscan_reset();
					stdscan_bufptr = special + n;
					tokval.t_type = TOKEN_INVALID;
					e = evaluate(stdscan, NULL, &tokval, &fwd, 0, NULL);
					if (fwd) {
						sym->nextfwd = fwds;
						fwds = sym;
						sym->name = StrHeap(name);
					} else if (e) {
						if (!is_simple(e))
							aasm_log(_LOG_ERROR, "cannot use relocatable"
								  " expression as symbol size");
						else
							sym->size = reloc_value(e);
					}
					stdscan_bufptr = saveme;    /* bugfix? fbk 8/10/00 */
				}
				special_used = true;
			}
			/*
			 * If TLS segment, mark symbol accordingly.
			 */
			if (sects[sym->section - 1]->flags & SHF_TLS) {
				sym->type &= 0xf0;
				sym->type |= STT_TLS;
			}
		}
		sym->globnum = nglobs;
		nglobs++;
	} else
		nlocals++;

	if (special && !special_used)
		aasm_log(_LOG_ERROR, "no special symbol features supported here");
}

static void elf_add_reloc(struct Section *sect, int32_t segment,
			  int64_t offset, int type)
{
	struct Reloc *r;
	r = *sect->tail = malcof(struct Reloc);
	sect->tail = &r->next;
	r->next = NULL;

	r->address = sect->len;
	r->offset = offset;
	if (segment == NO_SEG)
		r->symbol = 0;
	else {
		int i;
		r->symbol = 0;
		for (i = 0; i < nsects; i++)
			if (segment == sects[i]->index)
				r->symbol = i + 2;
		if (!r->symbol)
			r->symbol = GLOBAL_TEMP_BASE + StrpageGet(bsym, segment);
	}
	r->type = type;

	sect->nrelocs++;
}

/*
 * This routine deals with ..got and ..sym relocations: the more
 * complicated kinds. In shared-library writing, some relocations
 * with respect to global symbols must refer to the precise symbol
 * rather than referring to an offset from the base of the section
 * _containing_ the symbol. Such relocations call to this routine,
 * which searches the symbol list for the symbol in question.
 *
 * R_386_GOT32 references require the _exact_ symbol address to be
 * used; R_386_32 references can be at an offset from the symbol.
 * The boolean argument `exact' tells us this.
 *
 * Return value is the adjusted value of `addr', having become an
 * offset from the symbol rather than the section. Should always be
 * zero when returning from an exact call.
 *
 * Limitation: if you define two symbols at the same place,
 * confusion will occur.
 *
 * Inefficiency: we search, currently, using a linked list which
 * isn't even necessarily sorted.
 */
static void elf_add_gsym_reloc(struct Section *sect,
							   int32_t segment, uint64_t offset, int64_t pcrel,
				   int type, bool exact)
{
	struct Reloc *r;
	struct Section *s;
	struct Symbol *sym;
	struct rbtree *srb;
	int i;

	/*
	 * First look up the segment/offset pair and find a global
	 * symbol corresponding to it. If it's not one of our segments,
	 * then it must be an external symbol, in which case we're fine
	 * doing a normal elf_add_reloc after first sanity-checking
	 * that the offset from the symbol is zero.
	 */
	s = NULL;
	for (i = 0; i < nsects; i++)
		if (segment == sects[i]->index) {
			s = sects[i];
			break;
		}

	if (!s) {
	if (exact && offset)
		aasm_log(_LOG_ERROR, "invalid access to an external symbol");
	else 
		elf_add_reloc(sect, segment, offset - pcrel, type);
	return;
	}

	srb = s->gsyms->rb_search(offset);
	if (!srb || (exact && srb->key != offset)) {
	aasm_log(_LOG_ERROR, "unable to find a suitable global symbol"
		  " for this reference");
	return;
	}
	sym = container_of(srb, struct Symbol, symv);

	r = *sect->tail = malcof(Reloc);
	sect->tail = &r->next;
	r->next = NULL;

	r->address = sect->len;
	r->offset = offset - pcrel - sym->symv.key;
	r->symbol = GLOBAL_TEMP_BASE + sym->globnum;
	r->type = type;

	sect->nrelocs++;
}

static void elf_out(int32_t segto, const void *data,
			enum out_type type, uint64_t size,
					int32_t segment, int32_t wrt)
{
	struct Section *s;
	int64_t addr, zero;
	int i;
	static struct symlininfo sinfo;

	zero = 0;

#if defined(DEBUG) && DEBUG>2
	if (data) fprintf(stderr,
			" elf_out line: %d type: %x seg: %d segto: %d bytes: %x data: %"PRIx64"\n",
			   currentline, type, segment, segto, size, *(int64_t *)data);
	else fprintf(stderr,
			" elf_out line: %d type: %x seg: %d segto: %d bytes: %x\n",
			   currentline, type, segment, segto, size);
#endif

	/*
	 * handle absolute-assembly (structure definitions)
	 */
	if (segto == NO_SEG) {
		if (type != OUT_RESERVE)
			aasm_log(_LOG_ERROR, "attempt to assemble code in [ABSOLUTE]"
				  " space");
		return;
	}

	s = NULL;
	for (i = 0; i < nsects; i++)
		if (segto == sects[i]->index) {
			s = sects[i];
			break;
		}
	if (!s) {
		int tempint;            /* ignored */
		if (segto != elf_section_names(".text", 2, &tempint))
			aasm_log(_LOG_PANIC, "strange segment conditions in ELF driver");
		else {
			s = sects[nsects - 1];
			i = nsects - 1;
		}
	}
	/* invoke current debug_output routine */
	if (of_elf64.current_dfmt) {
		sinfo.offset = s->len;
		sinfo.section = i;
		sinfo.segto = segto;
		sinfo.name = s->name;
		of_elf64.current_dfmt->debug_output(TY_DEBUGSYMLIN, &sinfo);
	}
	/* end of debugging stuff */

	if (s->type == SHT_NOBITS && type != OUT_RESERVE) {
		aasm_log(_LOG_WARN, "attempt to initialize memory in"
			  " BSS section `%s': ignored", s->name);
		s->len += realsize(type, size);
		return;
	}

	if (type == OUT_RESERVE) {
		if (s->type == SHT_PROGBITS) {
			aasm_log(_LOG_WARN, "uninitialized space declared in"
				  " non-BSS section `%s': zeroing", s->name);
			elf_sect_write(s, NULL, size);
		} else
			s->len += size;
	} else if (type == OUT_RAWDATA) {
		if (segment != NO_SEG)
			aasm_log(_LOG_PANIC, "OUT_RAWDATA with other than NO_SEG");
		elf_sect_write(s, data, size);
	} else if (type == OUT_ADDRESS) {
		addr = *(int64_t *)data;
		if (segment == NO_SEG) {
		/* Do nothing */
	} else if (segment % 2) {
		aasm_log(_LOG_ERROR, "ELF format does not support"
		  " segment base references");
	} else {
		if (wrt == NO_SEG) {
		switch ((int)size) {
		case 1:
			elf_add_reloc(s, segment, addr, R_X86_64_8);
			break;
		case 2:
			elf_add_reloc(s, segment, addr, R_X86_64_16);
			break;
		case 4:
			elf_add_reloc(s, segment, addr, R_X86_64_32);
			break;
		case 8:
			elf_add_reloc(s, segment, addr, R_X86_64_64);
			break;
		default:
			aasm_log(_LOG_PANIC, "internal error elf64-hpa-871");
			break;
		}
		addr = 0;
		} else if (wrt == elf_gotpc_sect + 1) {
		/*
		 * The user will supply GOT relative to $$. ELF
		 * will let us have GOT relative to $. So we
		 * need to fix up the data item by $-$$.
		 */
		addr += s->len;
		elf_add_reloc(s, segment, addr, R_X86_64_GOTPC32);
		addr = 0;
		} else if (wrt == elf_gotoff_sect + 1) {
		if (size != 8) {
			aasm_log(_LOG_ERROR, "ELF64 requires ..gotoff "
			  "references to be qword");
		} else {
			elf_add_reloc(s, segment, addr, R_X86_64_GOTOFF64);
			addr = 0;
		}
		} else if (wrt == elf_got_sect + 1) {
		switch ((int)size) {
		case 4:
			elf_add_gsym_reloc(s, segment, addr, 0,
					   R_X86_64_GOT32, true);
			addr = 0;
			break;
		case 8:
			elf_add_gsym_reloc(s, segment, addr, 0,
					   R_X86_64_GOT64, true);
			addr = 0;
			break;
		default:
			aasm_log(_LOG_ERROR, "invalid ..got reference");
			break;
		}
		} else if (wrt == elf_sym_sect + 1) {
		switch ((int)size) {
		case 1:
			elf_add_gsym_reloc(s, segment, addr, 0,
					   R_X86_64_8, false);
			addr = 0;
			break;
		case 2:
			elf_add_gsym_reloc(s, segment, addr, 0,
					   R_X86_64_16, false);
			addr = 0;
			break;
		case 4:
			elf_add_gsym_reloc(s, segment, addr, 0,
					   R_X86_64_32, false);
			addr = 0;
			break;
		case 8:
			elf_add_gsym_reloc(s, segment, addr, 0,
					   R_X86_64_64, false);
			addr = 0;
			break;
		default:
			aasm_log(_LOG_PANIC, "internal error elf64-hpa-903");
			break;
		}
		} else if (wrt == elf_plt_sect + 1) {
		aasm_log(_LOG_ERROR, "ELF format cannot produce non-PC-"
			  "relative PLT references");
		} else {
		aasm_log(_LOG_ERROR, "ELF format does not support this"
			  " use of WRT");
		}
	}
	elf_sect_writeaddr(s, addr, size);
	} else if (type == OUT_REL2ADR) {
	addr = *(int64_t *)data - size;
		if (segment == segto)
			aasm_log(_LOG_PANIC, "intra-segment OUT_REL2ADR");
		if (segment == NO_SEG) {
		/* Do nothing */
	} else if (segment % 2) {
			aasm_log(_LOG_ERROR, "ELF format does not support"
				  " segment base references");
		} else {
			if (wrt == NO_SEG) {
				elf_add_reloc(s, segment, addr, R_X86_64_PC16);
		addr = 0;
			} else {
				aasm_log(_LOG_ERROR,
					  "Unsupported non-32-bit ELF relocation [2]");
			}
		}
	elf_sect_writeaddr(s, addr, 2);
	} else if (type == OUT_REL4ADR) {
	addr = *(int64_t *)data - size;
		if (segment == segto)
			aasm_log(_LOG_PANIC, "intra-segment OUT_REL4ADR");
		if (segment == NO_SEG) {
		/* Do nothing */
	} else if (segment % 2) {
			aasm_log(_LOG_ERROR, "ELF64 format does not support"
				  " segment base references");
		} else {
			if (wrt == NO_SEG) {
				elf_add_reloc(s, segment, addr, R_X86_64_PC32);
		addr = 0;
			} else if (wrt == elf_plt_sect + 1) {
				elf_add_gsym_reloc(s, segment, addr+size, size,
				   R_X86_64_PLT32, true);
		addr = 0;
			} else if (wrt == elf_gotpc_sect + 1 ||
			   wrt == elf_got_sect + 1) {
				elf_add_gsym_reloc(s, segment, addr+size, size,
				   R_X86_64_GOTPCREL, true);
		addr = 0;
			} else if (wrt == elf_gotoff_sect + 1 ||
			   wrt == elf_got_sect + 1) {
		aasm_log(_LOG_ERROR, "ELF64 requires ..gotoff references to be "
			  "qword absolute");
			} else if (wrt == elf_gottpoff_sect + 1) {
				elf_add_gsym_reloc(s, segment, addr+size, size,
				   R_X86_64_GOTTPOFF, true);
		addr = 0;
			} else {
				aasm_log(_LOG_ERROR, "ELF64 format does not support this"
					  " use of WRT");
			}
		}
	elf_sect_writeaddr(s, addr, 4);
	} else if (type == OUT_REL8ADR) {
	addr = *(int64_t *)data - size;
		if (segment == segto)
			aasm_log(_LOG_PANIC, "intra-segment OUT_REL8ADR");
		if (segment == NO_SEG) {
		/* Do nothing */
	} else if (segment % 2) {
			aasm_log(_LOG_ERROR, "ELF64 format does not support"
				  " segment base references");
		} else {
			if (wrt == NO_SEG) {
				elf_add_reloc(s, segment, addr, R_X86_64_PC64);
		addr = 0;
			} else if (wrt == elf_gotpc_sect + 1 ||
			   wrt == elf_got_sect + 1) {
				elf_add_gsym_reloc(s, segment, addr+size, size,
				   R_X86_64_GOTPCREL64, true);
		addr = 0;
			} else if (wrt == elf_gotoff_sect + 1 ||
			   wrt == elf_got_sect + 1) {
		aasm_log(_LOG_ERROR, "ELF64 requires ..gotoff references to be "
			  "absolute");
			} else if (wrt == elf_gottpoff_sect + 1) {
		aasm_log(_LOG_ERROR, "ELF64 requires ..gottpoff references to be "
			  "dword");
			} else {
				aasm_log(_LOG_ERROR, "ELF64 format does not support this"
					  " use of WRT");
			}
		}
		elf_sect_writeaddr(s, addr, 8);
	}
}

static void elf_write(void)
{
	int align;
	char *p;
	int i;

	Strbuff *symtab;
	int32_t symtablen, symtablocal;

	/*
	 * Work out how many sections we will have. We have SHN_UNDEF,
	 * then the flexible user sections, then the fixed sections
	 * `.shstrtab', `.symtab' and `.strtab', then optionally
	 * relocation sections for the user sections.
	 */
	nsections = sec_numspecial + 1;
	if (of_elf64.current_dfmt == &df_stabs)
		nsections += 3;
	else if (of_elf64.current_dfmt == &df_dwarf)
		nsections += 10;

	add_sectname("", ".shstrtab");
	add_sectname("", ".symtab");
	add_sectname("", ".strtab");
	for (i = 0; i < nsects; i++) {
		nsections++;            /* for the section itself */
		if (sects[i]->head) {
			nsections++;        /* for its relocations */
			add_sectname(".rela", sects[i]->name);
		}
	}

	if (of_elf64.current_dfmt == &df_stabs) {
		/* in case the debug information is wanted, just add these three sections... */
		add_sectname("", ".stab");
		add_sectname("", ".stabstr");
		add_sectname(".rel", ".stab");
	}

	else if (of_elf64.current_dfmt == &df_dwarf) {
		/* the dwarf debug standard specifies the following ten sections,
		   not all of which are currently implemented,
		   although all of them are defined. */
		#define debug_aranges (int64_t) (nsections-10)
		#define debug_info (int64_t) (nsections-7)
		#define debug_abbrev (int64_t) (nsections-5)
		#define debug_line (int64_t) (nsections-4)
		add_sectname("", ".debug_aranges");
		add_sectname(".rela", ".debug_aranges");
		add_sectname("", ".debug_pubnames");
		add_sectname("", ".debug_info");
		add_sectname(".rela", ".debug_info");
		add_sectname("", ".debug_abbrev");
		add_sectname("", ".debug_line");
		add_sectname(".rela", ".debug_line");
		add_sectname("", ".debug_frame");
		add_sectname("", ".debug_loc");
	}

	/*
	 * Output the ELF header.
	 */
	fwrite("\177ELF\2\1\1", 7, 1, elffp);
	fputc(elf_osabi, elffp);
	fputc(elf_abiver, elffp);
	fwritezero(7, elffp);
	fwriteint16_t(ET_REL, elffp);      /* relocatable file */
	fwriteint16_t(EM_X86_64, elffp);      /* processor ID */
	fwriteint32_t(1L, elffp);      /* EV_CURRENT file format version */
	fwriteint64_t(0L, elffp);      /* no entry point */
	fwriteint64_t(0L, elffp);      /* no program header table */
	fwriteint64_t(0x40L, elffp);   /* section headers straight after
								 * ELF header plus alignment */
	fwriteint32_t(0L, elffp);      /* 386 defines no special flags */
	fwriteint16_t(0x40, elffp);   /* size of ELF header */
	fwriteint16_t(0, elffp);      /* no program header table, again */
	fwriteint16_t(0, elffp);      /* still no program header table */
	fwriteint16_t(sizeof(Elf64_Shdr), elffp);   /* size of section header */
	fwriteint16_t(nsections, elffp);      /* number of sections */
	fwriteint16_t(sec_shstrtab, elffp);   /* string table section index for
					   * section header table */

	/*
	 * Build the symbol table and relocation tables.
	 */
	symtab = elf_build_symtab(&symtablen, &symtablocal);
	for (i = 0; i < nsects; i++)
		if (sects[i]->head)
			sects[i]->rel = elf_build_reltab(&sects[i]->rellen,
											 sects[i]->head);

	/*
	 * Now output the section header table.
	 */

	elf_foffs = 0x40 + sizeof(Elf64_Shdr) * nsections;
	align = ((elf_foffs + SEG_ALIGN_1) & ~SEG_ALIGN_1) - elf_foffs;
	elf_foffs += align;
	elf_nsect = 0;
	elf_sects = (ELF_SECTDATA*)malc(sizeof(*elf_sects) * nsections);

	/* SHN_UNDEF */
	elf_section_header(0, SHT_NULL, 0, NULL, false, 0, SHN_UNDEF, 0, 0, 0);
	p = shstrtab + 1;

	/* The normal sections */
	for (i = 0; i < nsects; i++) {
		elf_section_header(p - shstrtab, sects[i]->type, sects[i]->flags,
						   (sects[i]->type == SHT_PROGBITS ?
							sects[i]->data : NULL), true,
						   sects[i]->len, 0, 0, sects[i]->align, 0);
		p += StrLength(p) + 1;
	}

	/* .shstrtab */
	elf_section_header(p - shstrtab, SHT_STRTAB, 0, shstrtab, false,
			   shstrtablen, 0, 0, 1, 0);
	p += StrLength(p) + 1;

	/* .symtab */
	elf_section_header(p - shstrtab, SHT_SYMTAB, 0, symtab, true,
			   symtablen, sec_strtab, symtablocal, 4, 24);
	p += StrLength(p) + 1;

	/* .strtab */
	elf_section_header(p - shstrtab, SHT_STRTAB, 0, strs, true,
			   strslen, 0, 0, 1, 0);
	p += StrLength(p) + 1;

	/* The relocation sections */
	for (i = 0; i < nsects; i++)
		if (sects[i]->head) {
			elf_section_header(p - shstrtab, SHT_RELA, 0, sects[i]->rel, true,
							   sects[i]->rellen, sec_symtab, i + 1, 4, 24);
			p += StrLength(p) + 1;
		}

	if (of_elf64.current_dfmt == &df_stabs) {
		/* for debugging information, create the last three sections
		   which are the .stab , .stabstr and .rel.stab sections respectively */

		/* this function call creates the stab sections in memory */
		stabs64_generate();

		if (stabbuf && stabstrbuf && stabrelbuf) {
			elf_section_header(p - shstrtab, SHT_PROGBITS, 0, stabbuf, false,
				   stablen, sec_stabstr, 0, 4, 12);
			p += StrLength(p) + 1;

			elf_section_header(p - shstrtab, SHT_STRTAB, 0, stabstrbuf, false,
							   stabStrLength, 0, 0, 4, 0);
			p += StrLength(p) + 1;

			/* link -> symtable  info -> section to refer to */
			elf_section_header(p - shstrtab, SHT_REL, 0, stabrelbuf, false,
							   stabrellen, symtabsection, sec_stab, 4, 16);
			p += StrLength(p) + 1;
		}
	}
	else if (of_elf64.current_dfmt == &df_dwarf) {
			/* for dwarf debugging information, create the ten dwarf sections */

			/* this function call creates the dwarf sections in memory */
			if (dwarf_fsect)
		dwarf64_generate();

			elf_section_header(p - shstrtab, SHT_PROGBITS, 0, arangesbuf, false,
							   arangeslen, 0, 0, 1, 0);
			p += StrLength(p) + 1;

			elf_section_header(p - shstrtab, SHT_RELA, 0, arangesrelbuf, false,
							   arangesrellen, symtabsection, debug_aranges, 1, 24);
			p += StrLength(p) + 1;

			elf_section_header(p - shstrtab, SHT_PROGBITS, 0, pubnamesbuf, false,
							   pubnameslen, 0, 0, 1, 0);
			p += StrLength(p) + 1;

			elf_section_header(p - shstrtab, SHT_PROGBITS, 0, infobuf, false,
							   infolen, 0, 0, 1, 0);
			p += StrLength(p) + 1;

			elf_section_header(p - shstrtab, SHT_RELA, 0, inforelbuf, false,
							   inforellen, symtabsection, debug_info, 1, 24);
			p += StrLength(p) + 1;

			elf_section_header(p - shstrtab, SHT_PROGBITS, 0, abbrevbuf, false,
							   abbrevlen, 0, 0, 1, 0);
			p += StrLength(p) + 1;

			elf_section_header(p - shstrtab, SHT_PROGBITS, 0, linebuf, false,
							   linelen, 0, 0, 1, 0);
			p += StrLength(p) + 1;

			elf_section_header(p - shstrtab, SHT_RELA, 0, linerelbuf, false,
							   linerellen, symtabsection, debug_line, 1, 24);
			p += StrLength(p) + 1;

			elf_section_header(p - shstrtab, SHT_PROGBITS, 0, framebuf, false,
							   framelen, 0, 0, 8, 0);
			p += StrLength(p) + 1;

			elf_section_header(p - shstrtab, SHT_PROGBITS, 0, locbuf, false,
							   loclen, 0, 0, 1, 0);
			p += StrLength(p) + 1;
	}
	fwritezero(align, elffp);

	/*
	 * Now output the sections.
	 */
	elf_write_sections();

	memf(elf_sects);
	StrbuffFree(symtab);
}

static Strbuff *elf_build_symtab(int32_t *len, int32_t *local)
{
	Strbuff *s = StrbuffNew(1L);
	struct Symbol *sym;
	uint8_t entry[24], *p;
	int i;

	*len = *local = 0;

	/*
	 * First, an all-zeros entry, required by the ELF spec.
	 */
	StrbuffSend(s, NULL, 24L);   /* null symbol table entry */
	*len += 24;
	(*local)++;

	/*
	 * Next, an entry for the file name.
	 */
	p = entry;
	WRITELONG(p, 1);            /* we know it's 1st entry in strtab */
	WRITESHORT(p, STT_FILE);    /* type FILE */
	WRITESHORT(p, SHN_ABS);
	WRITEDLONG(p, (uint64_t) 0);  /* no value */
	WRITEDLONG(p, (uint64_t) 0);  /* no size either */
	StrbuffSend(s, entry, 24L);
	*len += 24;
	(*local)++;

	/*
	 * Now some standard symbols defining the segments, for relocation
	 * purposes.
	 */
	for (i = 1; i <= nsects; i++) {
		p = entry;
		WRITELONG(p, 0);        /* no symbol name */
		WRITESHORT(p, STT_SECTION);       /* type, binding, and visibility */
		WRITESHORT(p, i);       /* section id */
		WRITEDLONG(p, (uint64_t) 0);        /* offset zero */
		WRITEDLONG(p, (uint64_t) 0);        /* size zero */
		StrbuffSend(s, entry, 24L);
		*len += 24;
		(*local)++;
	}


	/*
	 * Now the other local symbols.
	 */
	StrbuffRewind(syms);
	while ((sym = (Symbol*)StrbuffEread(syms))) {
		if (sym->type & SYM_GLOBAL)
			continue;
		p = entry;
		WRITELONG(p, sym->strpos);	/* index into symbol string table */
		WRITECHAR(p, sym->type);        /* type and binding */
		WRITECHAR(p, sym->other);	/* visibility */
		WRITESHORT(p, sym->section);    /* index into section header table */
		WRITEDLONG(p, (int64_t)sym->symv.key); /* value of symbol */
		WRITEDLONG(p, (int64_t)sym->size);  /* size of symbol */
		StrbuffSend(s, entry, 24L);
		*len += 24;
		(*local)++;
	}
	 /*
	  * dwarf needs symbols for debug sections
	  * which are relocation targets.
	  */  
	 if (of_elf64.current_dfmt == &df_dwarf) {
		dwarf_infosym = *local;
		p = entry;
		WRITELONG(p, 0);        /* no symbol name */
		WRITESHORT(p, STT_SECTION);       /* type, binding, and visibility */
		WRITESHORT(p, debug_info);       /* section id */
		WRITEDLONG(p, (uint64_t) 0);        /* offset zero */
		WRITEDLONG(p, (uint64_t) 0);        /* size zero */
		StrbuffSend(s, entry, 24L);
		*len += 24;
		(*local)++;
		dwarf_abbrevsym = *local;
		p = entry;
		WRITELONG(p, 0);        /* no symbol name */
		WRITESHORT(p, STT_SECTION);       /* type, binding, and visibility */
		WRITESHORT(p, debug_abbrev);       /* section id */
		WRITEDLONG(p, (uint64_t) 0);        /* offset zero */
		WRITEDLONG(p, (uint64_t) 0);        /* size zero */
		StrbuffSend(s, entry, 24L);
		*len += 24;
		(*local)++;
		dwarf_linesym = *local;
		p = entry;
		WRITELONG(p, 0);        /* no symbol name */
		WRITESHORT(p, STT_SECTION);       /* type, binding, and visibility */
		WRITESHORT(p, debug_line);       /* section id */
		WRITEDLONG(p, (uint64_t) 0);        /* offset zero */
		WRITEDLONG(p, (uint64_t) 0);        /* size zero */
		StrbuffSend(s, entry, 24L);
		*len += 24;
		(*local)++;
	 }

	/*
	 * Now the global symbols.
	 */
	StrbuffRewind(syms);
	while ((sym = (Symbol*)StrbuffEread(syms))) {
		if (!(sym->type & SYM_GLOBAL))
			continue;
		p = entry;
		WRITELONG(p, sym->strpos);
		WRITECHAR(p, sym->type);        /* type and binding */
		WRITECHAR(p, sym->other);       /* visibility */
		WRITESHORT(p, sym->section);
		WRITEDLONG(p, (int64_t)sym->symv.key);
		WRITEDLONG(p, (int64_t)sym->size);
		StrbuffSend(s, entry, 24L);
		*len += 24;
	}

	return s;
}

static Strbuff* elf_build_reltab(uint64_t* len, struct Reloc* r)
{
	Strbuff* s;
	uint8_t* p, entry[24];
	int32_t global_offset;

	if (!r)
		return NULL;

	s = StrbuffNew(1L);
	*len = 0;

	/*
	 * How to onvert from a global placeholder to a real symbol index;
	 * the +2 refers to the two special entries, the null entry and
	 * the filename entry.
	 */
	global_offset = -GLOBAL_TEMP_BASE + nsects + nlocals + ndebugs + 2;

	while (r) {
		int32_t sym = r->symbol;

		if (sym >= GLOBAL_TEMP_BASE)
			sym += global_offset;

		p = entry;
		WRITEDLONG(p, r->address);
		WRITELONG(p, r->type);
		WRITELONG(p, sym);
		WRITEDLONG(p, r->offset);
		StrbuffSend(s, entry, 24L);
		*len += 24;

		r = r->next;
	}

	return s;
}

static void elf_section_header(int name, int type, uint64_t flags,
							   void *data, bool is_saa, uint64_t datalen,
							   int link, int info, int align, int eltsize)
{
	elf_sects[elf_nsect].data = data;
	elf_sects[elf_nsect].len = datalen;
	elf_sects[elf_nsect].is_saa = is_saa;
	elf_nsect++;

	fwriteint32_t((int32_t)name, elffp);
	fwriteint32_t((int32_t)type, elffp);
	fwriteint64_t((int64_t)flags, elffp);
	fwriteint64_t(0L, elffp);      /* no address, ever, in object files */
	fwriteint64_t(type == 0 ? 0L : elf_foffs, elffp);
	fwriteint64_t(datalen, elffp);
	if (data)
		elf_foffs += (datalen + SEG_ALIGN_1) & ~SEG_ALIGN_1;
	fwriteint32_t((int32_t)link, elffp);
	fwriteint32_t((int32_t)info, elffp);
	fwriteint64_t((int64_t)align, elffp);
	fwriteint64_t((int64_t)eltsize, elffp);
}

static void elf_write_sections(void)
{
	int i;
	for0(i, elf_nsect) if (elf_sects[i].data) {
		int32_t len = elf_sects[i].len;
		int32_t reallen = (len + SEG_ALIGN_1) & ~SEG_ALIGN_1;
		int32_t align = reallen - len;
		if (elf_sects[i].is_saa)
			StrbuffSendFile((Strbuff*)elf_sects[i].data, elffp);
		else
			fwrite(elf_sects[i].data, len, 1, elffp);
		fwritezero(align, elffp);
	}
}

static void elf_sect_write(struct Section *sect, const void *data, size_t len)
{
	StrbuffSend(sect->data, (pureptr_t)data, len);
	sect->len += len;
}
static void elf_sect_writeaddr(struct Section *sect, int64_t data, size_t len)
{
	StrbuffSend(sect->data, &data, len);
	sect->len += len;
}

static int32_t elf_segbase(int32_t segment)
{
	return segment;
}

static int elf_directive(char *directive, char *value, int pass)
{
	bool err = 0;
	int64_t n;
	char *p;

	if (!StrCompare(directive, "osabi")) {
	if (pass == 2)
		return 1;		/* ignore in pass 2 */

	n = readnum(value, &err);
	if (err) {
		aasm_log(_LOG_ERROR, "`osabi' directive requires a parameter");
		return 1;
	}
	if (n < 0 || n > 255) {
		aasm_log(_LOG_ERROR, "valid osabi numbers are 0 to 255");
		return 1;
	}
	elf_osabi  = n;
	elf_abiver = 0;

	if ((p = (char*)StrIndexChar(value,',')) == NULL)
		return 1;

	n = readnum(p+1, &err);
	if (err || n < 0 || n > 255) {
		aasm_log(_LOG_ERROR, "invalid ABI version number (valid: 0 to 255)");
		return 1;
	}
	
	elf_abiver = n;
	return 1;
	}
	
	return 0;
}

static void elf_filename(char *inname, char *outname)
{
	StrCopy(elf_module, inname);
	standard_extension(inname, outname, ".o");
}

static int elf_set_info(enum geninfo type, char **val)
{
	(void)type;
	(void)val;
	return 0;
}


/* common debugging routines */
static void debug64_deflabel(char *name, int32_t segment, int64_t offset,
				 int is_global, char *special)
{
	(void)name;
	(void)segment;
	(void)offset;
	(void)is_global;
	(void)special;
}

static void debug64_directive(const char *directive, const char *params)
{
	(void)directive;
	(void)params;
}

static void debug64_typevalue(int32_t type)
{
	int32_t stype, ssize;
	switch (TYM_TYPE(type)) {
		case TY_LABEL:
			ssize = 0;
			stype = STT_NOTYPE;
			break;
		case TY_BYTE:
			ssize = 1;
			stype = STT_OBJECT;
			break;
		case TY_WORD:
			ssize = 2;
			stype = STT_OBJECT;
			break;
		case TY_DWORD:
			ssize = 4;
			stype = STT_OBJECT;
			break;
		case TY_FLOAT:
			ssize = 4;
			stype = STT_OBJECT;
			break;
		case TY_QWORD:
			ssize = 8;
			stype = STT_OBJECT;
			break;
		case TY_TBYTE:
			ssize = 10;
			stype = STT_OBJECT;
			break;
		case TY_OWORD:
			ssize = 16;
			stype = STT_OBJECT;
			break;
		case TY_YWORD:
		ssize = 32;
		stype = STT_OBJECT;
		break;
		case TY_COMMON:
			ssize = 0;
			stype = STT_COMMON;
			break;
		case TY_SEG:
			ssize = 0;
			stype = STT_SECTION;
			break;
		case TY_EXTERN:
			ssize = 0;
			stype = STT_NOTYPE;
			break;
		case TY_EQU:
			ssize = 0;
			stype = STT_NOTYPE;
			break;
		default:
			ssize = 0;
			stype = STT_NOTYPE;
			break;
	}
	if (stype == STT_OBJECT && lastsym && !lastsym->type) {
		lastsym->size = ssize;
		lastsym->type = stype;
	}
}

/* stabs debugging routines */

static void stabs64_linenum(const char *filename, int32_t linenumber, int32_t segto)
{
	(void)segto;
	if (!stabs_filename) {
		stabs_filename = (char *)malc(StrLength(filename) + 1);
		StrCopy(stabs_filename, filename);
	} else {
		if (StrCompare(stabs_filename, filename)) {
			/* yep, a memory leak...this program is one-shot anyway, so who cares...
			   in fact, this leak comes in quite handy to maintain a list of files
			   encountered so far in the symbol lines... */

			/* why not memf(stabs_filename); we're done with the old one */

			stabs_filename = (char *)malc(StrLength(filename) + 1);
			StrCopy(stabs_filename, filename);
		}
	}
	debug_immcall = 1;
	currentline = linenumber;
}


static void stabs64_output(int type, void *param)
{
	struct symlininfo *s;
	struct linelist *el;
	if (type == TY_DEBUGSYMLIN) {
		if (debug_immcall) {
			s = (struct symlininfo *)param;
			if (!(sects[s->section]->flags & SHF_EXECINSTR))
				return; /* line info is only collected for executable sections */
			numlinestabs++;
			el = (struct linelist *)malc(sizeof(struct linelist));
			el->info.offset = s->offset;
			el->info.section = s->section;
			el->info.name = s->name;
			el->line = currentline;
			el->filename = stabs_filename;
			el->next = 0;
			if (stabslines) {
				stabslines->last->next = el;
				stabslines->last = el;
			} else {
				stabslines = el;
				stabslines->last = el;
			}
		}
	}
	debug_immcall = 0;
}

#define WRITE_STAB(p,n_strx,n_type,n_other,n_desc,n_value) \
  do {\
	WRITELONG(p,n_strx); \
	WRITECHAR(p,n_type); \
	WRITECHAR(p,n_other); \
	WRITESHORT(p,n_desc); \
	WRITELONG(p,n_value); \
  } while (0)

/* for creating the .stab , .stabstr and .rel.stab sections in memory */

static void stabs64_generate(void)
{
	int i, numfiles, strsize, numstabs = 0, currfile, mainfileindex;
	uint8_t *sbuf, *ssbuf, *rbuf, *sptr, *rptr;
	char **allfiles;
	int *fileidx;

	struct linelist *ptr;

	ptr = stabslines;

	allfiles = (char **)malc(numlinestabs * sizeof(int8_t *));
	for (i = 0; i < numlinestabs; i++)
		allfiles[i] = 0;
	numfiles = 0;
	while (ptr) {
		if (numfiles == 0) {
			allfiles[0] = ptr->filename;
			numfiles++;
		} else {
			for (i = 0; i < numfiles; i++) {
				if (!StrCompare(allfiles[i], ptr->filename))
					break;
			}
			if (i >= numfiles) {
				allfiles[i] = ptr->filename;
				numfiles++;
			}
		}
		ptr = ptr->next;
	}
	strsize = 1;
	fileidx = (int *)malc(numfiles * sizeof(int));
	for (i = 0; i < numfiles; i++) {
		fileidx[i] = strsize;
		strsize += StrLength(allfiles[i]) + 1;
	}
	mainfileindex = 0;
	for (i = 0; i < numfiles; i++) {
		if (!StrCompare(allfiles[i], elf_module)) {
			mainfileindex = i;
			break;
		}
	}

	/* worst case size of the stab buffer would be:
	   the sourcefiles changes each line, which would mean 1 SOL, 1 SYMLIN per line
	 */
	sbuf =
		(uint8_t *)malc((numlinestabs * 2 + 3) *
									 sizeof(struct stabentry));

	ssbuf = (uint8_t *)malc(strsize);

	rbuf = (uint8_t *)malc(numlinestabs * 16 * (2 + 3));
	rptr = rbuf;

	for (i = 0; i < numfiles; i++) {
		StrCopy((char *)ssbuf + fileidx[i], allfiles[i]);
	}
	ssbuf[0] = 0;

	stabStrLength = strsize;       /* set global variable for length of stab strings */

	sptr = sbuf;
	ptr = stabslines;
	numstabs = 0;

	if (ptr) {
		/* this is the first stab, its strx points to the filename of the
		the source-file, the n_desc field should be set to the number
		of remaining stabs
		*/
		WRITE_STAB(sptr, fileidx[0], 0, 0, 0, StrLength(allfiles[0] + 12));

		/* this is the stab for the main source file */
		WRITE_STAB(sptr, fileidx[mainfileindex], N_SO, 0, 0, 0);

		/* relocation table entry */

		/* Since the symbol table has two entries before */
		/* the section symbols, the index in the info.section */
		/* member must be adjusted by adding 2 */

		WRITEDLONG(rptr, (int64_t)(sptr - sbuf) - 4);
	WRITELONG(rptr, R_X86_64_32);
	WRITELONG(rptr, ptr->info.section + 2);

		numstabs++;
		currfile = mainfileindex;
	}

	while (ptr) {
		if (StrCompare(allfiles[currfile], ptr->filename)) {
			/* oops file has changed... */
			for (i = 0; i < numfiles; i++)
				if (!StrCompare(allfiles[i], ptr->filename))
					break;
			currfile = i;
			WRITE_STAB(sptr, fileidx[currfile], N_SOL, 0, 0,
					   ptr->info.offset);
			numstabs++;

			/* relocation table entry */

			WRITEDLONG(rptr, (int64_t)(sptr - sbuf) - 4);
		WRITELONG(rptr, R_X86_64_32);
		WRITELONG(rptr, ptr->info.section + 2);
		}

		WRITE_STAB(sptr, 0, N_SLINE, 0, ptr->line, ptr->info.offset);
		numstabs++;

		/* relocation table entry */

		WRITEDLONG(rptr, (int64_t)(sptr - sbuf) - 4);
	WRITELONG(rptr, R_X86_64_32);
	WRITELONG(rptr, ptr->info.section + 2);

		ptr = ptr->next;

	}

	((struct stabentry *)sbuf)->n_desc = numstabs;

	memf(allfiles);
	memf(fileidx);

	stablen = (sptr - sbuf);
	stabrellen = (rptr - rbuf);
	stabrelbuf = rbuf;
	stabbuf = sbuf;
	stabstrbuf = ssbuf;
}

static void stabs64_cleanup(void)
{
	struct linelist *ptr, *del;
	if (!stabslines)
		return;
	ptr = stabslines;
	while (ptr) {
		del = ptr;
		ptr = ptr->next;
		memf(del);
	}
	if (stabbuf)
		memf(stabbuf);
	if (stabrelbuf)
		memf(stabrelbuf);
	if (stabstrbuf)
		memf(stabstrbuf);
}
/* dwarf routines */
static void dwarf64_init(struct outffmt *of, void *id, FILE * fp)
{
	(void)of;
	(void)id;
	(void)fp;

	ndebugs = 3;		/* 3 debug symbols */
}

static void dwarf64_linenum(const char *filename, int32_t linenumber,
				int32_t segto)
{
	(void)segto;
	dwarf64_findfile(filename);
	debug_immcall = 1;
	currentline = linenumber;
}

/* called from elf_out with type == TY_DEBUGSYMLIN */
static void dwarf64_output(int type, void *param)
{
  int ln, aa, inx, maxln, soc;
  struct symlininfo *s;
  Strbuff *plinep;

  (void)type;

  s = (struct symlininfo *)param;
   /* line number info is only gathered for executable sections */
   if (!(sects[s->section]->flags & SHF_EXECINSTR))
	 return;
  /* Check if section index has changed */
  if (!(dwarf_csect && (dwarf_csect->section) == (s->section)))
  {
	 dwarf64_findsect(s->section);
  }
  /* do nothing unless line or file has changed */
  if (debug_immcall)
  {
	ln = currentline - dwarf_csect->line;
	aa = s->offset - dwarf_csect->offset;
	inx = dwarf_clist->line;
	plinep = dwarf_csect->psaa;
	/* check for file change */
	if (!(inx == dwarf_csect->file))
	{
	   StrbuffSend8(plinep,DW_LNS_set_file);
	   StrbuffSend8(plinep,inx);
	   dwarf_csect->file = inx;
	}
	/* check for line change */
	if (ln)
	{
	   /* test if in range of special op code */
	   maxln = line_base + line_range;
	   soc = (ln - line_base) + (line_range * aa) + opcode_base;
	   if (ln >= line_base && ln < maxln && soc < 256)
	   {
		  StrbuffSend8(plinep,soc);
	   }
	   else
	   {
		  if (ln)
		  {
		  StrbuffSend8(plinep,DW_LNS_advance_line);
		  StrbuffSendWleb128s(plinep,ln);
		  }
		  if (aa)
		  {
		  StrbuffSend8(plinep,DW_LNS_advance_pc);
		  StrbuffSendWleb128u(plinep,aa);
		  }
	   }
	   dwarf_csect->line = currentline;
	   dwarf_csect->offset = s->offset;
	}
	/* show change handled */
	debug_immcall = 0;
  }
}


static void dwarf64_generate(void)
{
	uint8_t* pbuf;
	int indx;
	struct linelist* ftentry;
	Strbuff* paranges, * ppubnames, * pinfo, * pabbrev, * plines, * plinep;
	Strbuff* parangesrel, * plinesrel, * pinforel;
	struct sectlist* psect;
	size_t saalen, linepoff, totlen, highaddr;

	/* write epilogues for each line program range */
	/* and build aranges section */
	paranges = StrbuffNew(1L);
	parangesrel = StrbuffNew(1L);
	StrbuffSend16(paranges, 3);		/* dwarf version */
	StrbuffSend64(parangesrel, paranges->total_len + 4);
	StrbuffSend64(parangesrel, (dwarf_infosym << 32) + R_X86_64_32); /* reloc to info */
	StrbuffSend64(parangesrel, 0);
	StrbuffSend32(paranges, 0);		/* offset into info */
	StrbuffSend8(paranges, 8);		/* pointer size */
	StrbuffSend8(paranges, 0);		/* not segmented */
	StrbuffSend32(paranges, 0);		/* padding */
	/* iterate though sectlist entries */
	psect = dwarf_fsect;
	totlen = 0;
	highaddr = 0;
	for (indx = 0; indx < dwarf_nsections; indx++)
	{
		plinep = psect->psaa;
		/* Line Number Program Epilogue */
		StrbuffSend8(plinep, 2);			/* std op 2 */
		StrbuffSend8(plinep, (sects[psect->section]->len) - psect->offset);
		StrbuffSend8(plinep, DW_LNS_extended_op);
		StrbuffSend8(plinep, 1);			/* operand length */
		StrbuffSend8(plinep, DW_LNE_end_sequence);
		totlen += plinep->total_len;
		/* range table relocation entry */
		StrbuffSend64(parangesrel, paranges->total_len + 4);
		StrbuffSend64(parangesrel, ((uint64_t)(psect->section + 2) << 32) + R_X86_64_64);
		StrbuffSend64(parangesrel, (uint64_t)0);
		/* range table entry */
		StrbuffSend64(paranges, 0x0000);		/* range start */
		StrbuffSend64(paranges, sects[psect->section]->len);	/* range length */
		highaddr += sects[psect->section]->len;
		/* done with this entry */
		psect = psect->next;
	}
	StrbuffSend64(paranges, 0);		/* null address */
	StrbuffSend64(paranges, 0);		/* null length */
	saalen = paranges->total_len;
	arangeslen = saalen + 4;
	arangesbuf = pbuf = (uint8*)malc(arangeslen);
	WRITELONG(pbuf, saalen);			/* initial length */
	StrbuffPop(paranges, pbuf, saalen);
	StrbuffFree(paranges);

	/* build rela.aranges section */
	arangesrellen = saalen = parangesrel->total_len;
	arangesrelbuf = pbuf = (uint8*)malc(arangesrellen);
	StrbuffPop(parangesrel, pbuf, saalen);
	StrbuffFree(parangesrel);

	/* build pubnames section */
	ppubnames = StrbuffNew(1L);
	StrbuffSend16(ppubnames, 3);			/* dwarf version */
	StrbuffSend32(ppubnames, 0);			/* offset into info */
	StrbuffSend32(ppubnames, 0);			/* space used in info */
	StrbuffSend32(ppubnames, 0);			/* end of list */
	saalen = ppubnames->total_len;
	pubnameslen = saalen + 4;
	pubnamesbuf = pbuf = (uint8*)malc(pubnameslen);
	WRITELONG(pbuf, saalen);	/* initial length */
	StrbuffPop(ppubnames, pbuf, saalen);
	StrbuffFree(ppubnames);

	/* build info section */
	pinfo = StrbuffNew(1L);
	pinforel = StrbuffNew(1L);
	StrbuffSend16(pinfo, 3);			/* dwarf version */
	StrbuffSend64(pinforel, pinfo->total_len + 4);
	StrbuffSend64(pinforel, (dwarf_abbrevsym << 32) + R_X86_64_32); /* reloc to abbrev */
	StrbuffSend64(pinforel, 0);
	StrbuffSend32(pinfo, 0);			/* offset into abbrev */
	StrbuffSend8(pinfo, 8);			/* pointer size */
	StrbuffSend8(pinfo, 1);			/* abbrviation number LEB128u */
	StrbuffSend64(pinforel, pinfo->total_len + 4);
	StrbuffSend64(pinforel, ((uint64_t)(dwarf_fsect->section + 2) << 32) + R_X86_64_64);
	StrbuffSend64(pinforel, 0);
	StrbuffSend64(pinfo, 0);			/* DW_AT_low_pc */
	StrbuffSend64(pinforel, pinfo->total_len + 4);
	StrbuffSend64(pinforel, ((uint64_t)(dwarf_fsect->section + 2) << 32) + R_X86_64_64);
	StrbuffSend64(pinforel, 0);
	StrbuffSend64(pinfo, highaddr);		/* DW_AT_high_pc */
	StrbuffSend64(pinforel, pinfo->total_len + 4);
	StrbuffSend64(pinforel, (dwarf_linesym << 32) + R_X86_64_32); /* reloc to line */
	StrbuffSend64(pinforel, 0);
	StrbuffSend32(pinfo, 0);			/* DW_AT_stmt_list */
	StrbuffSend(pinfo, elf_module, StrLength(elf_module) + 1);
	StrbuffSend(pinfo, (pureptr_t)aasm_signature, StrLength(aasm_signature) + 1);
	StrbuffSend16(pinfo, DW_LANG_Mips_Assembler);
	StrbuffSend8(pinfo, 2);			/* abbrviation number LEB128u */
	StrbuffSend64(pinforel, pinfo->total_len + 4);
	StrbuffSend64(pinforel, ((uint64_t)(dwarf_fsect->section + 2) << 32) + R_X86_64_64);
	StrbuffSend64(pinforel, 0);
	StrbuffSend64(pinfo, 0);			/* DW_AT_low_pc */
	StrbuffSend64(pinfo, 0);			/* DW_AT_frame_base */
	StrbuffSend8(pinfo, 0);			/* end of entries */
	saalen = pinfo->total_len;
	infolen = saalen + 4;
	infobuf = pbuf = (uint8*)malc(infolen);
	WRITELONG(pbuf, saalen);		/* initial length */
	StrbuffPop(pinfo, pbuf, saalen);
	StrbuffFree(pinfo);

	/* build rela.info section */
	inforellen = saalen = pinforel->total_len;
	inforelbuf = pbuf = (uint8*)malc(inforellen);
	StrbuffPop(pinforel, pbuf, saalen);
	StrbuffFree(pinforel);

	/* build abbrev section */
	pabbrev = StrbuffNew(1L);
	StrbuffSend8(pabbrev, 1);			/* entry number LEB128u */
	StrbuffSend8(pabbrev, DW_TAG_compile_unit);	/* tag LEB128u */
	StrbuffSend8(pabbrev, 1);			/* has children */
	/* the following attributes and forms are all LEB128u values */
	StrbuffSend8(pabbrev, DW_AT_low_pc);
	StrbuffSend8(pabbrev, DW_FORM_addr);
	StrbuffSend8(pabbrev, DW_AT_high_pc);
	StrbuffSend8(pabbrev, DW_FORM_addr);
	StrbuffSend8(pabbrev, DW_AT_stmt_list);
	StrbuffSend8(pabbrev, DW_FORM_data4);
	StrbuffSend8(pabbrev, DW_AT_name);
	StrbuffSend8(pabbrev, DW_FORM_string);
	StrbuffSend8(pabbrev, DW_AT_producer);
	StrbuffSend8(pabbrev, DW_FORM_string);
	StrbuffSend8(pabbrev, DW_AT_language);
	StrbuffSend8(pabbrev, DW_FORM_data2);
	StrbuffSend16(pabbrev, 0);			/* end of entry */
	/* LEB128u usage same as above */
	StrbuffSend8(pabbrev, 2);			/* entry number */
	StrbuffSend8(pabbrev, DW_TAG_subprogram);
	StrbuffSend8(pabbrev, 0);			/* no children */
	StrbuffSend8(pabbrev, DW_AT_low_pc);
	StrbuffSend8(pabbrev, DW_FORM_addr);
	StrbuffSend8(pabbrev, DW_AT_frame_base);
	StrbuffSend8(pabbrev, DW_FORM_data4);
	StrbuffSend16(pabbrev, 0);			/* end of entry */
	abbrevlen = saalen = pabbrev->total_len;
	abbrevbuf = pbuf = (uint8*)malc(saalen);
	StrbuffPop(pabbrev, pbuf, saalen);
	StrbuffFree(pabbrev);

	/* build line section */
	/* prolog */
	plines = StrbuffNew(1L);
	StrbuffSend8(plines, 1);			/* Minimum Instruction Length */
	StrbuffSend8(plines, 1);			/* Initial value of 'is_stmt' */
	StrbuffSend8(plines, line_base);		/* Line Base */
	StrbuffSend8(plines, line_range);	/* Line Range */
	StrbuffSend8(plines, opcode_base);	/* Opcode Base */
	/* standard opcode lengths (# of LEB128u operands) */
	StrbuffSend8(plines, 0);			/* Std opcode 1 length */
	StrbuffSend8(plines, 1);			/* Std opcode 2 length */
	StrbuffSend8(plines, 1);			/* Std opcode 3 length */
	StrbuffSend8(plines, 1);			/* Std opcode 4 length */
	StrbuffSend8(plines, 1);			/* Std opcode 5 length */
	StrbuffSend8(plines, 0);			/* Std opcode 6 length */
	StrbuffSend8(plines, 0);			/* Std opcode 7 length */
	StrbuffSend8(plines, 0);			/* Std opcode 8 length */
	StrbuffSend8(plines, 1);			/* Std opcode 9 length */
	StrbuffSend8(plines, 0);			/* Std opcode 10 length */
	StrbuffSend8(plines, 0);			/* Std opcode 11 length */
	StrbuffSend8(plines, 1);			/* Std opcode 12 length */
	/* Directory Table */
	StrbuffSend8(plines, 0);			/* End of table */
	/* File Name Table */
	ftentry = dwarf_flist;
	for (indx = 0;indx < dwarf_numfiles;indx++)
	{
		StrbuffSend(plines, ftentry->filename, (int32_t)(StrLength(ftentry->filename) + 1));
		StrbuffSend8(plines, 0);			/* directory  LEB128u */
		StrbuffSend8(plines, 0);			/* time LEB128u */
		StrbuffSend8(plines, 0);			/* size LEB128u */
		ftentry = ftentry->next;
	}
	StrbuffSend8(plines, 0);			/* End of table */
	linepoff = plines->total_len;
	linelen = linepoff + totlen + 10;
	linebuf = pbuf = (uint8*)malc(linelen);
	WRITELONG(pbuf, linelen - 4);		/* initial length */
	WRITESHORT(pbuf, 3);			/* dwarf version */
	WRITELONG(pbuf, linepoff);		/* offset to line number program */
	/* write line header */
	saalen = linepoff;
	StrbuffPop(plines, pbuf, saalen);   /* read a given no. of bytes */
	pbuf += linepoff;
	StrbuffFree(plines);
	/* concatonate line program ranges */
	linepoff += 13;
	plinesrel = StrbuffNew(1L);
	psect = dwarf_fsect;
	for (indx = 0; indx < dwarf_nsections; indx++)
	{
		StrbuffSend64(plinesrel, linepoff);
		StrbuffSend64(plinesrel, ((uint64_t)(psect->section + 2) << 32) + R_X86_64_64);
		StrbuffSend64(plinesrel, (uint64_t)0);
		plinep = psect->psaa;
		saalen = plinep->total_len;
		StrbuffPop(plinep, pbuf, saalen);
		pbuf += saalen;
		linepoff += saalen;
		StrbuffFree(plinep);
		/* done with this entry */
		psect = psect->next;
	}


	/* build rela.lines section */
	linerellen = saalen = plinesrel->total_len;
	linerelbuf = pbuf = (uint8*)malc(linerellen);
	StrbuffPop(plinesrel, pbuf, saalen);
	StrbuffFree(plinesrel);

	/* build frame section */
	framelen = 4;
	framebuf = pbuf = (uint8*)malc(framelen);
	WRITELONG(pbuf, framelen - 4);		/* initial length */

	/* build loc section */
	loclen = 16;
	locbuf = pbuf = (uint8*)malc(loclen);
	WRITEDLONG(pbuf, 0);		/* null  beginning offset */
	WRITEDLONG(pbuf, 0);		/* null  ending offset */
}

static void dwarf64_cleanup(void)
{
	if (arangesbuf)
		memf(arangesbuf);
	if (arangesrelbuf)
		memf(arangesrelbuf);
	if (pubnamesbuf)
		memf(pubnamesbuf);
	if (infobuf)
		memf(infobuf);
	if (inforelbuf)
		memf(inforelbuf);
	if (abbrevbuf)
		memf(abbrevbuf);
	if (linebuf)
		memf(linebuf);
	if (linerelbuf)
		memf(linerelbuf);
	if (framebuf)
		memf(framebuf);
	if (locbuf)
		memf(locbuf);
}
static void dwarf64_findfile(const char* fname)
{
	int finx;
	struct linelist* match;

	/* return if fname is current file name */
	if (dwarf_clist && !(StrCompare(fname, dwarf_clist->filename))) return;
	/* search for match */
	else
	{
		match = 0;
		if (dwarf_flist)
		{
			match = dwarf_flist;
			for (finx = 0; finx < dwarf_numfiles; finx++)
			{
				if (!(StrCompare(fname, match->filename)))
				{
					dwarf_clist = match;
					return;
				}
			}
		}
		/* add file name to end of list */
		dwarf_clist = (struct linelist*)malc(sizeof(struct linelist));
		dwarf_numfiles++;
		dwarf_clist->line = dwarf_numfiles;
		dwarf_clist->filename = (char*)malc(StrLength(fname) + 1);
		StrCopy(dwarf_clist->filename, fname);
		dwarf_clist->next = 0;
		/* if first entry */
		if (!dwarf_flist)
		{
			dwarf_flist = dwarf_elist = dwarf_clist;
			dwarf_clist->last = 0;
		}
		/* chain to previous entry */
		else
		{
			dwarf_elist->next = dwarf_clist;
			dwarf_elist = dwarf_clist;
		}
	}
}
/*  */
static void dwarf64_findsect(const int index)
{
	int sinx;
	struct sectlist* match;
	Strbuff* plinep;
	/* return if index is current section index */
	if (dwarf_csect && (dwarf_csect->section == index))
	{
		return;
	}
	/* search for match */
	else
	{
		match = 0;
		if (dwarf_fsect)
		{
			match = dwarf_fsect;
			for (sinx = 0; sinx < dwarf_nsections; sinx++)
			{
				if ((match->section == index))
				{
					dwarf_csect = match;
					return;
				}
				match = match->next;
			}
		}
		/* add entry to end of list */
		dwarf_csect = (struct sectlist*)malc(sizeof(struct sectlist));
		dwarf_nsections++;
		dwarf_csect->psaa = plinep = StrbuffNew(1L);
		dwarf_csect->line = 1;
		dwarf_csect->offset = 0;
		dwarf_csect->file = 1;
		dwarf_csect->section = index;
		dwarf_csect->next = 0;
		/* set relocatable address at start of line program */
		StrbuffSend8(plinep, DW_LNS_extended_op);
		StrbuffSend8(plinep, 9);			/* operand length */
		StrbuffSend8(plinep, DW_LNE_set_address);
		StrbuffSend64(plinep, 0);		/* Start Address */
		/* if first entry */
		if (!dwarf_fsect)
		{
			dwarf_fsect = dwarf_esect = dwarf_csect;
			dwarf_csect->last = 0;
		}
		/* chain to previous entry */
		else
		{
			dwarf_esect->next = dwarf_csect;
			dwarf_esect = dwarf_csect;
		}
	}
}

