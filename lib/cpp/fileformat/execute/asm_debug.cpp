// ASCII C TAB4 CRLF
// Docutitle: (Format.Executable) 
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

#ifndef _MCCA

// ---- ---- AASM ---- ---- //

#include "../../../../inc/c/strbuff.h"
#include "../../../../inc/c/strpage.h"
#include "../../../../asm/inc/aasm.h"


#ifdef OF_DBG

struct Section {
	struct Section *next;
	int32_t number;
	char *name;
} *dbgsect;

FILE *dbgf;

static void dbg_init(FILE* fp, ldfunc ldef, evalfunc eval);
static void dbg_deflabel(char* name, int32_t segment, int64_t offset, int is_global, char* special);
static int32_t dbg_section_names(rostr name, int pass, int* bits);
static int32_t dbg_segbase(int32_t segment);
static int dbg_set_info(enum geninfo type, char** val);
static void dbg_out(int32_t segto, const void* data, enum out_type type, uint64_t size, int32_t segment, int32_t wrt);
static int dbg_directive(char* directive, char* value, int pass);
static void dbg_filename(char* inname, char* outname);
static void dbg_cleanup(int debuginfo);

void dbgdbg_init(struct outffmt* of, void* id, FILE* fp);
static void dbgdbg_linnum(const char* lnfname, int32_t lineno, int32_t segto);
static void dbgdbg_deflabel(char* name, int32_t segment, int64_t offset, int is_global, char* special);
static void dbgdbg_define(const char* type, const char* params);
static void dbgdbg_typevalue(int32_t type);
static void dbgdbg_output(int output_type, void* param);
static void dbgdbg_cleanup(void);

static struct dbgffmt debug_debug_form = {
	"Trace of all info passed to debug stage",
	"debug",
	dbgdbg_init,
	dbgdbg_linnum,
	dbgdbg_deflabel,
	dbgdbg_define,
	dbgdbg_typevalue,
	dbgdbg_output,
	dbgdbg_cleanup,
};

static struct dbgffmt* debug_debug_arr[3] = {
	&debug_debug_form,
	&null_debug_form,
	NULL
};

struct outffmt of_dbg = {
	"Trace of all info passed to output stage",
	"dbg",
	OFMT_TEXT,
	debug_debug_arr,
	&debug_debug_form,
	NULL,
	dbg_init,
	dbg_set_info,
	dbg_out,
	dbg_deflabel,
	dbg_section_names,
	dbg_segbase,
	dbg_directive,
	dbg_filename,
	dbg_cleanup
};

static void dbg_init(FILE * fp, ldfunc ldef, evalfunc eval)
{
	(void)eval;

	dbgf = fp;
	dbgsect = NULL;
	(void)ldef;
	fprintf(fp, "NASM Output format debug dump\n");
}

static void dbg_cleanup(int debuginfo)
{
	(void)debuginfo;
	of_dbg.current_dfmt->cleanup();
	while (dbgsect) {
		struct Section *tmp = dbgsect;
		dbgsect = dbgsect->next;
		memf(tmp->name);
		memf(tmp);
	}
	fclose(dbgf);
}

static int32_t dbg_section_names(rostr name, int pass, int *bits)
{
	int seg;

	/*
	 * We must have an initial default: let's make it 16.
	 */
	if (!name)
		*bits = 16;

	if (!name)
		fprintf(dbgf, "section_name on init: returning %d\n",
				seg = SegAlloc());
	else {
		int n = StrSpanExclude(name, " \t");
		char *sname = StrHeapN(name, n);
		struct Section *s;

		seg = NO_SEG;
		for (s = dbgsect; s; s = s->next)
			if (!StrCompare(s->name, sname))
				seg = s->number;

		if (seg == NO_SEG) {
			s = (Section*)malc(sizeof(*s));
			s->name = sname;
			s->number = seg = SegAlloc();
			s->next = dbgsect;
			dbgsect = s;
			fprintf(dbgf, "section_name %s (pass %d): returning %d\n",
					name, pass, seg);
		}
	}
	return seg;
}

static void dbg_deflabel(char *name, int32_t segment, int64_t offset, int is_global, char *special)
{
	fprintf(dbgf, "deflabel %s := %08" PRIx32 ":%016" PRIx64 " %s (%d)%s%s\n",
			name, segment, offset,
			is_global == 2 ? "common" : is_global ? "global" : "local",
			is_global, special ? ": " : "", special);
}

static void dbg_out(int32_t segto, const void* data, enum out_type type, uint64_t size, int32_t segment, int32_t wrt)
{
	int32_t ldata;
	int id;

	fprintf(dbgf, "out to %" PRIx32 ", len = %" PRIu64 ": ", segto, size);

	switch (type) {
	case OUT_RESERVE:
		fprintf(dbgf, "reserved.\n");
		break;
	case OUT_RAWDATA:
		fprintf(dbgf, "raw data = ");
		while (size--) {
			id = *(uint8_t*)data;
			data = (char*)data + 1;
			fprintf(dbgf, "%02x ", id);
		}
		fprintf(dbgf, "\n");
		break;
	case OUT_ADDRESS:
		ldata = *(int64_t*)data;
		fprintf(dbgf, "addr %08" PRIx32 " (seg %08" PRIx32 ", wrt %08" PRIx32 ")\n", ldata,
			segment, wrt);
		break;
	case OUT_REL2ADR:
		fprintf(dbgf, "rel2adr %04" PRIx16 " (seg %08" PRIx32 ")\n",
			(uint16_t) * (int64_t*)data, segment);
		break;
	case OUT_REL4ADR:
		fprintf(dbgf, "rel4adr %08" PRIx32 " (seg %08" PRIx32 ")\n",
			(uint32_t) * (int64_t*)data, segment);
		break;
	case OUT_REL8ADR:
		fprintf(dbgf, "rel8adr %016" PRIx64 " (seg %08" PRIx32 ")\n",
			(uint64_t) * (int64_t*)data, segment);
		break;
	default:
		fprintf(dbgf, "unknown\n");
		break;
	}
}

static int32_t dbg_segbase(int32_t segment)
{
	return segment;
}

static int dbg_directive(char *directive, char *value, int pass)
{
	fprintf(dbgf, "directive [%s] value [%s] (pass %d)\n",
			directive, value, pass);
	return 1;
}

static void dbg_filename(char *inname, char *outname)
{
	standard_extension(inname, outname, ".dbg");
}

static int dbg_set_info(enum geninfo type, char **val)
{
	(void)type;
	(void)val;
	return 0;
}

rostr types[] = {
	"unknown", "label", "byte", "word", "dword", "float", "qword", "tbyte"
};
void dbgdbg_init(struct outffmt *of, void *id, FILE * fp)
{
	(void)of;
	(void)id;
	(void)fp;
	fprintf(fp, "   With debug info\n");
}
static void dbgdbg_cleanup(void)
{
}

static void dbgdbg_linnum(const char *lnfname, int32_t lineno, int32_t segto)
{
	fprintf(dbgf, "dbglinenum %s(%" PRId32 ") := %08" PRIx32 "\n",
		lnfname, lineno, segto);
}
static void dbgdbg_deflabel(char* name, int32_t segment, int64_t offset, int is_global, char* special)
{
	fprintf(dbgf, "dbglabel %s := %08" PRIx32 ":%016" PRIx64 " %s (%d)%s%s\n",
		name,
		segment, offset,
		is_global == 2 ? "common" : is_global ? "global" : "local",
		is_global, special ? ": " : "", special);
}
static void dbgdbg_define(const char *type, const char *params)
{
	fprintf(dbgf, "dbgdirective [%s] value [%s]\n", type, params);
}
static void dbgdbg_output(int output_type, void *param)
{
	(void)output_type;
	(void)param;
}
static void dbgdbg_typevalue(int32_t type)
{
	fprintf(dbgf, "new type: %s(%" PRIX32 ")\n",
			types[TYM_TYPE(type) >> 3], TYM_ELEMENTS(type));
}


#endif                          /* OF_DBG */
#endif
