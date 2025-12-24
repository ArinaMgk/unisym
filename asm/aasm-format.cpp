// UTF-8 
// @ArinaMgk
// OutFileFormat

#include "inc/aasm.h"

outffmt* OutfileFormat::crt_format = NULL;

static int ndrivers = 0;

struct outffmt* drivers[] = {
	&of_bin,
	&of_ith,
	&of_srec,
	&of_aout,
	&of_aoutb,
	&of_coff,
	&of_elf32,
	&of_elf,
	&of_elf64,
	&of_as86,
	&of_obj,
	&of_win32,
	&of_win64,
	// &of_rdf2,
	&of_ieee,
	&of_macho32,
	&of_macho,
	&of_macho64,
	&of_dbg,
	NULL
};

#ifndef OF_DEFAULT
#define OF_DEFAULT of_bin
#endif

uint64_t realsize(enum out_type type, uint64_t size)
{
	switch (type) {
	case OUT_REL2ADR:
		return 2;
	case OUT_REL4ADR:
		return 4;
	case OUT_REL8ADR:
		return 8;
	default:
		return size;
	}
}

struct outffmt* ofmt_find(char* name)
{                               /* find driver */
	int i;

	for (i = 0; i < ndrivers; i++)
		if (!StrCompare(name, drivers[i]->shortname))
			return drivers[i];

	return NULL;
}
struct dbgffmt* dfmt_find(struct outffmt* ofmt, char* name)
{                               /* find driver */
	struct dbgffmt** dfmt = ofmt->debug_formats;
	while (*dfmt) {
		if (!StrCompare(name, (*dfmt)->shortname))
			return (*dfmt);
		dfmt++;
	}
	return NULL;
}

void ofmt_list(struct outffmt* deffmt, FILE* fp)
{
	int i;
	for (i = 0; i < ndrivers; i++)
		fprintf(fp, "  %c %-10s%s\n",
			drivers[i] == deffmt ? '*' : ' ',
			drivers[i]->shortname, drivers[i]->fullname);
}
void dfmt_list(struct outffmt* ofmt, FILE* fp)
{
	struct dbgffmt** drivers = ofmt->debug_formats;
	while (*drivers) {
		fprintf(fp, "  %c %-10s%s\n",
			drivers[0] == ofmt->current_dfmt ? '*' : ' ',
			drivers[0]->shortname, drivers[0]->fullname);
		drivers++;
	}
}

struct outffmt* ofmt_register()
{
	// foreach format driver
	for (ndrivers = 0; drivers[ndrivers] != NULL; ndrivers++);
	if (ndrivers == 0) {
		log_0file = 1; aasm_log(_LOG_PANIC, "No output drivers given at compile time");
	}
	return (&OF_DEFAULT);
}


// ----

// register_output_formats = ofmt_register
void OutfileFormat::setMode() {
	crt_format = ofmt_register();
}

_ESYM_C struct outffmt** ofmt_f() {
	return &OutfileFormat::crt_format;
}
