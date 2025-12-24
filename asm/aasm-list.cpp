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

#include "inc/aasm.h"
#include "../inc/c/stdinc.h"





#define LIST_MAX_LEN 216        /* something sensible */
#define LIST_INDENT  40
#define LIST_HEXBIT  18

typedef struct MacroInhibit MacroInhibit;

static struct MacroInhibit {
	MacroInhibit *next;
	int level;
	int inhibiting;
} *mistack;

static char xdigit[] = "0123456789ABCDEF";

#define HEX(a,b) (*(a)=xdigit[((b)>>4)&15],(a)[1]=xdigit[(b)&15]);

static char listline[LIST_MAX_LEN];
static bool listlinep;

static char listerror[LIST_MAX_LEN];

static char listdata[2 * LIST_INDENT];  /* we need less than that actually */
static int32_t listoffset;

static int32_t listlineno;

static int32_t listp;

static int suppress;            /* for INCBIN & TIMES special cases */

static int listlevel, listlevel_e;

static FILE *listfp;

static void list_emit(void)
{
	int i;

	if (!listlinep && !listdata[0])
		return;

	fprintf(listfp, "%6" PRId32 " ", ++listlineno);

	if (listdata[0])
		fprintf(listfp, "%08" PRIX32 " %-*s", listoffset, LIST_HEXBIT + 1,
				listdata);
	else
		fprintf(listfp, "%*s", LIST_HEXBIT + 10, "");

	if (listlevel_e)
		fprintf(listfp, "%s<%d>", (listlevel < 10 ? " " : ""),
				listlevel_e);
	else if (listlinep)
		fprintf(listfp, "    ");

	if (listlinep)
		fprintf(listfp, " %s", listline);

	putc('\n', listfp);
	listlinep = false;
	listdata[0] = '\0';

	if (listerror[0]) {
	fprintf(listfp, "%6" PRId32 "          ", ++listlineno);
	for (i = 0; i < LIST_HEXBIT; i++)
		putc('*', listfp);
	
	if (listlevel_e)
		fprintf(listfp, " %s<%d>", (listlevel < 10 ? " " : ""),
			listlevel_e);
	else
		fprintf(listfp, "     ");

	fprintf(listfp, "  %s\n", listerror);
	listerror[0] = '\0';
	}
}

static void list_init(char *fname)
{
	listfp = fopen(fname, "w");
	if (!listfp) {
		//--auto clean flag
		aasm_log(_LOG_ERROR, "unable to open listing file `%s'", fname);
		return;
	}

	*listline = '\0';
	listlineno = 0;
	*listerror = '\0';
	listp = true;
	listlevel = 0;
	suppress = 0;
	mistack = malcof(MacroInhibit);
	mistack->next = NULL;
	mistack->level = 0;
	mistack->inhibiting = true;
}

static void list_cleanup(void)
{
	if (!listp)
		return;

	while (mistack) {
		MacroInhibit *temp = mistack;
		mistack = temp->next;
		memf(temp);
	}

	list_emit();
	fclose(listfp);
}

static void list_out(int32_t offset, char *str)
{
	if (StrLength(listdata) + StrLength(str) > LIST_HEXBIT) {
		StrAppend(listdata, "-");
		list_emit();
	}
	if (!listdata[0])
		listoffset = offset;
	StrAppend(listdata, str);
}

static void list_output(int32_t offset, const void *data,
			enum out_type type, uint64_t size)
{
	if (!listp || suppress || user_nolist)      /* fbk - 9/2/00 */
		return;

	switch (type) {
	case OUT_RAWDATA:
	{
		uint8_t const *p = (uint8_t const*)data;
		char q[3];
	if (size == 0 && !listdata[0])
		listoffset = offset;
		while (size--) {
			HEX(q, *p);
			q[2] = '\0';
			list_out(offset++, q);
			p++;
		}
	break;
	}
	case OUT_ADDRESS:
	{
		uint64_t d = *(int64_t *)data;
		char q[20];
		uint8_t p[8], *r = p;
		if (size == 4) {
			q[0] = '[';
			q[9] = ']';
			q[10] = '\0';
			WRITELONG(r, d); //(uni::cast<void*>(r), d);
			HEX(q + 1, p[0]);
			HEX(q + 3, p[1]);
			HEX(q + 5, p[2]);
			HEX(q + 7, p[3]);
			list_out(offset, q);
		} else if (size == 8) {
			q[0] = '[';
			q[17] = ']';
			q[18] = '\0';
			WRITEDLONG(r, d); //(uni::cast<void*>(r), d);
			HEX(q + 1,  p[0]);
			HEX(q + 3,  p[1]);
			HEX(q + 5,  p[2]);
			HEX(q + 7,  p[3]);
			HEX(q + 9,  p[4]);
			HEX(q + 11, p[5]);
			HEX(q + 13, p[6]);
			HEX(q + 15, p[7]);
			list_out(offset, q);
		} else {
			q[0] = '[';
			q[5] = ']';
			q[6] = '\0';
			WRITESHORT(r, d); //(uni::cast<void*>(r), d);
			HEX(q + 1, p[0]);
			HEX(q + 3, p[1]);
			list_out(offset, q);
		}
	break;
	}
	case OUT_REL2ADR:
	{
		uint32_t d = *(int32_t *)data;
		char q[11];
		uint8_t p[4], *r = p;
		q[0] = '(';
		q[5] = ')';
		q[6] = '\0';
		WRITESHORT(r, d); //(uni::cast<void*>(r), d);
		HEX(q + 1, p[0]);
		HEX(q + 3, p[1]);
		list_out(offset, q);
	break;
	}
	case OUT_REL4ADR:
	{
		uint32_t d = *(int32_t *)data;
		char q[11];
		uint8_t p[4], *r = p;
		q[0] = '(';
		q[9] = ')';
		q[10] = '\0';
		WRITELONG(r, d); //(uni::cast<void*>(r), d);
		HEX(q + 1, p[0]);
		HEX(q + 3, p[1]);
		HEX(q + 5, p[2]);
		HEX(q + 7, p[3]);
		list_out(offset, q);
	break;
	}
	case OUT_REL8ADR:
	{
		uint64_t d = *(int64_t *)data;
		char q[19];
		uint8_t p[8], *r = p;
		q[0] = '(';
		q[17] = ')';
		q[18] = '\0';
		WRITEDLONG(r, d); //(uni::cast<void*>(r), d);
		HEX(q + 1, p[0]);
		HEX(q + 3, p[1]);
		HEX(q + 5, p[2]);
		HEX(q + 7, p[3]);
		HEX(q + 9, p[4]);
		HEX(q + 11, p[5]);
		HEX(q + 13, p[6]);
		HEX(q + 15, p[7]);
		list_out(offset, q);
	break;
	}
	case OUT_RESERVE:
	{
		char q[20];
		snprintf(q, sizeof(q), "<res %08" PRIX64 ">", size);
		list_out(offset, q);
	break;
	}
	}
}

static void list_line(int type, char *line)
{
	if (!listp)
		return;
	if (user_nolist) {          /* fbk - 9/2/00 */
		listlineno++;
		return;
	}

	if (mistack && mistack->inhibiting) {
		if (type == LIST_MACRO)
			return;
		else {                  /* pop the m i stack */
			MacroInhibit *temp = mistack;
			mistack = temp->next;
			memf(temp);
		}
	}
	list_emit();
	listlinep = true;
	StrCopyN(listline, line, LIST_MAX_LEN - 1);
	listline[LIST_MAX_LEN - 1] = '\0';
	listlevel_e = listlevel;
}

static void list_uplevel(int type)
{
	if (!listp)
		return;
	if (type == LIST_INCBIN || type == LIST_TIMES) {
		suppress |= (type == LIST_INCBIN ? 1 : 2);
		list_out(listoffset, (char*)(type == LIST_INCBIN ? "<incbin>" : "<rept>"));
		return;
	}

	listlevel++;

	if (mistack && mistack->inhibiting && type == LIST_INCLUDE) {
		MacroInhibit *temp = malcof(MacroInhibit);
		temp->next = mistack;
		temp->level = listlevel;
		temp->inhibiting = false;
		mistack = temp;
	} else if (type == LIST_MACRO_NOLIST) {
		MacroInhibit *temp = malcof(MacroInhibit);
		temp->next = mistack;
		temp->level = listlevel;
		temp->inhibiting = true;
		mistack = temp;
	}
}

static void list_downlevel(int type)
{
	if (!listp)
		return;

	if (type == LIST_INCBIN || type == LIST_TIMES) {
		suppress &= ~(type == LIST_INCBIN ? 1 : 2);
		return;
	}

	listlevel--;
	while (mistack && mistack->level > listlevel) {
		MacroInhibit *temp = mistack;
		mistack = temp->next;
		memf(temp);
	}
}

static void list_error(loglevel_t severity, const char *pfx, const char *msg)
{
	if (!listfp)
	return;

	snprintf(listerror, sizeof listerror, "%s%s", pfx, msg);

	if (severity == _LOG_FATAL)
	list_emit();
}

ListGen aasmlist = {
	list_init,
	list_cleanup,
	list_output,
	list_line,
	list_uplevel,
	list_downlevel,
	list_error
};
