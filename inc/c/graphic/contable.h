// GBK @HRN.RFR23+.
// DOS/CONSOLE I/O MATRIX TABLE (considering Excel Software).
// Open-source ArinaMgk
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#ifndef ModConTable
#define ModConTable
typedef struct Conitem
{
	char* content;
	unsigned prop;// ~0 means default
} Conitem;

typedef struct Contab
{
	unsigned* rows, * cols;
	unsigned defa;// default prop
	Conitem** cons;
	unsigned curc, curr;
	int (*CtabItemResetVerify)(unsigned x, unsigned y, const char* buf);
	int (*CtabItemEnterEdit)(unsigned x, unsigned y);// return 1 for OKEY
	void (*CtabItemAfterEdit)(unsigned x, unsigned y);
} Contab;
extern char CtabExKey;
extern char CtabEdKey;

struct Contab* CtabMakeQuick(const char*** strs, unsigned* rows, unsigned* cols);

void CtabPrint(struct Contab* ct);

void CtabRemove(struct Contab* ct);

void CtabCursor(struct Contab* ct);

unsigned CtabRowCount(struct Contab* ct);
unsigned CtabColCount(struct Contab* ct);

int CtabLoop(struct Contab* ctab);

Conitem* CtabItemMakeQuick(const char** str, size_t strcount, size_t count);

// INSERT FUNCTIONS
void CtabAppendRowFore(struct Contab* ctab, struct Conitem* citm, unsigned rowsp);
void CtabAppendRowBack(struct Contab* ctab, struct Conitem* citm, unsigned rowsp);
void CtabAppendColFore(struct Contab* ctab, struct Conitem* citm, unsigned colsp);
void CtabAppendColBack(struct Contab* ctab, struct Conitem* citm, unsigned colsp);

void CtabRemoveRow(struct Contab* ctab);
void CtabRemoveCol(struct Contab* ctab);

#endif // !ModConTable
