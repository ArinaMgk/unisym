// GBK @HRN.RFR23+.
// DOS/CONSOLE I/O MATRIX TABLE (considering Excel Software).
// Open-source ArinaMgk yo FR28
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
// RFR30 FIX MISTAKE YO _CtabPrint

#define _CRT_SECURE_NO_WARNINGS
#ifndef _MCCA // Bare Metal C
#include <stdio.h>

#if defined(_WinNT)
#include <conio.h>
#else // ifdef _Linux
inline static int _getch() { char ch = getchar(); printf("\b "); return ch; }
char *gets_s(char *buffer, size_t sizeInCharacters);
#endif
#include "../../inc/c/graphic/contable.h"
#include "../../inc/c/stdinc.h"

#include "../../inc/c/ustring.h"
#include "../../inc/c/consio.h"
// 
typedef unsigned addr_t;

char CtabExKey = 'q';// CtabExitKey
char CtabEdKey = 'i';// CtabEditKey

struct Contab* CtabMakeQuick(const char*** strs, unsigned* rows, unsigned* cols)
{
	int automake = strs == 0;
	size_t colnum = 1, rownum = 1;
	while (rows[rownum]) rownum++; rownum--;
	while (cols[colnum]) colnum++; colnum--;
	struct Contab* dest; memalloc(dest, sizeof(struct Contab));
	if (!dest) return 0;
	// at least an item
	memalloc(dest->cons, (sizeof(struct Conitem*)) * (rownum + 1));
	if (!dest->cons)
	{
		memfree(dest);
		return 0;
	}
	// Copy text content
	for (size_t i = 0; i < rownum; i++)
	{
		memalloc(dest->cons[i], (sizeof(struct Conitem)) * (colnum + 1));
		if (!dest->cons[i])
		{
			memfree(dest);
			//TODO. There should be complex judge.
			return 0;
		}
		for (size_t j = 0; j < colnum; j++)
		{
			dest->cons[i][j].content = StrHeap(automake ? "" : strs[i][j]);
		}
	}
	dest->rows = MemHeap(rows, (sizeof(unsigned)) * (rownum + 2));
	dest->cols = MemHeap(cols, (sizeof(unsigned)) * (colnum + 2));
	dest->rows[rownum + 1] = 0;
	dest->cols[colnum + 1] = 0;
	dest->curc = dest->curr = 0;
	dest->CtabItemResetVerify = 0;
	return dest;
}

void CtabPrint(struct Contab* ct)// may be overwritten
{
	char c;
	if (!ct) return;
	for (size_t i = 0; !i || ct->rows[i + 1] > 0; i++)
	{
		ConCursor(0, ct->rows[i]);
		for (size_t k = 0; k < ct->cols[0]; k++)
		{
			c = ct->cons[i]->prop == 0x5A ? '>' : ' ';// optional
			putchar('>');
		}
		for (size_t j = 0; !j || ct->cols[j + 1] > 0; j++)
		{
			ConCursor(ct->cols[j], ct->rows[i]);
			if (StrLength(ct->cons[i][j].content) >= ct->cols[j + 1] - ct->cols[j])
			{
				for (size_t l = 0; l < ct->cols[ct->curc + 1] - ct->cols[ct->curc] - 4; l++)
					putchar(ct->cons[i][j].content[l]);
				putchar(' ');
				printf("...");
			}
			else
			{
				unsigned int k = printf("%s", ct->cons[i][j].content);
				for (; k < ct->cols[j + 1] - ct->cols[j]; k++) putchar(' ');
			}
		}
	}
	CtabCursor(ct);
}

void CtabRemove(struct Contab* ct)
{
	if (!ct) return;
	for (size_t i = 0; !i || ct->rows[i + 1] > 0; i++)
	{
		for (size_t j = 0; !j || ct->cols[j + 1] > 0; j++)
		{
			memfree(ct->cons[i][j].content);
		}
		memfree(ct->cons[i]);
	}
	memfree(ct->cols);
	memfree(ct->rows);
	memfree(ct->cons);
	memfree(ct);
}

void CtabCursor(struct Contab* ct)
{
	size_t colnum = 1, rownum = 1;
	while (ct->rows[rownum]) rownum++;
	while (ct->cols[colnum]) colnum++;
	while (ct->curr >= rownum)ct->curr -= rownum;
	while (ct->curc >= colnum)ct->curc -= colnum;

	ConCursor(ct->cols[ct->curc], ct->rows[ct->curr]);
}

unsigned CtabRowCount(struct Contab* ct)
{
	size_t rownum = 1;
	while (ct->rows[rownum]) rownum++; rownum--;
	return rownum;
}

unsigned CtabColCount(struct Contab* ct)
{
	size_t colnum = 1;
	while (ct->cols[colnum]) colnum++; colnum--;
	return colnum;
}

int CtabLoop(struct Contab* ctab)
{
	int ord;
	int mask = 0;
	int RW = 1;
	rewind(stdin), ord = _getch();
	switch (ord)
	{
	case 224:// ARINA GUESS 2 BYTES MADE~
		mask = 1;
		switch (ord = _getch())// now assignment for debug.
		{
		case 72:// UP
			if (ctab->curr)
				ctab->curr--;
			else ctab->curr = CtabRowCount(ctab) - 1;
			break;
		case 75:// LEFT
			if (ctab->curc)
				ctab->curc--;
			else ctab->curc = CtabColCount(ctab) - 1;
			break;
		case 77:// RIGHT
			if (ctab->curc < CtabColCount(ctab) - 1)
				ctab->curc++;
			else ctab->curc = 0;
			break;
		case 80:// DOWN
			if (ctab->curr < CtabRowCount(ctab) - 1)
				ctab->curr++;
			else ctab->curr = 0;
			break;
		default:
			//printf("%d", ord);// na dbg
			break;
		}
		CtabCursor(ctab);
		break;

	default:
		if (ord == CtabExKey) break;
		else if (ord == CtabEdKey && (!ctab->CtabItemEnterEdit || (ctab->CtabItemEnterEdit(ctab->curc, ctab->curr))))
		{
			char* buf;// δ��
			memalloc(buf, 128);
			char* p = buf;
			rewind(stdin);
			#ifdef _Linux
			{char* tmp = fgets(buf, 64, stdin);}
			#else
			gets_s(buf, 64);// mgk number
			#endif
			// RFR25: this will set the rest bytes zero? --Phina.
			if ((!ctab->CtabItemResetVerify) || ctab->CtabItemResetVerify(ctab->curc, ctab->curr, buf))
			srs(ctab->cons[ctab->curr][ctab->curc].content, StrHeapN(buf, 62));// mgk number
			memfree(buf);
			if (ctab->CtabItemAfterEdit) ctab->CtabItemAfterEdit(ctab->curc, ctab->curr);
			{// ENTER
				if (ctab->curr < CtabRowCount(ctab) - 1)
					ctab->curr++;
				else ctab->curr = 0;
			} CtabPrint(ctab);
		}
		break;
	}

	return mask ? (-ord) : ord;
}

Conitem* CtabItemMakeQuick(const char** str, size_t strcount, size_t count)
{
	if (!str || !count) return 0;
	Conitem* ret;
	memalloc(ret, sizeof(Conitem) * count);
	for (size_t i = 0; i < minof(strcount, count); i++)
	{
		ret[i].content = StrHeap(str[i]);
	}
	for (size_t i = minof(strcount, count); i < count; i++)
	{
		ret[i].content = StrHeap("");
	}
	return ret;
}

void CtabAppendRowFore(struct Contab* ctab, struct Conitem* citm, unsigned rowsp)
{
	// rowsp is not rows!
	if (!ctab || !citm) return;
	// move the current and the latter items late
	addr_t newridx = ctab->curr;
	addr_t newrowcount = CtabRowCount(ctab) + 1;
	// remake .rows and .cons[:]

	unsigned* NewRows;
	memalloc(NewRows, (newrowcount + 2) * sizeof(unsigned));
	// MemCopyN((char*)NewRows, (const char*)ctab->rows, newridx * sizeof(unsigned));
	for (unsigned i = 0; i < newridx; i++)
	{
		NewRows[i] = ctab->rows[i];
	}
	NewRows[newridx] = (newridx > 0 ? ctab->rows[newridx] : ctab->rows[0]);
	for (unsigned i = newridx + 1; i < newrowcount + 1; i++)
	{
		NewRows[i] = ctab->rows[i - 1] + rowsp;
	}
	NewRows[newrowcount + 1] = 0;
	srs(ctab->rows, NewRows);

	Conitem** NewCon;
	memalloc(NewCon, (newrowcount) * sizeof(struct Conitem*));
	MemCopyN((char*)NewCon, (const char*)ctab->cons, newridx * sizeof(struct Conitem*));
	NewCon[newridx] = citm;
	for (unsigned i = newridx + 1; i < newrowcount; i++)
	{
		NewCon[i] = ctab->cons[i - 1];
	}
	srs(ctab->cons, NewCon);

	ctab->curr++;// point to the previous
}

void CtabAppendRowBack(struct Contab* ctab, struct Conitem* citm, unsigned rowsp)
{
	// Based on the above.
	if (!ctab || !citm) return;
	// move the current and the latter items late
	addr_t newridx = ctab->curr + 1;
	addr_t newrowcount = CtabRowCount(ctab) + 1;
	// remake .rows and .cons[:]

	unsigned* NewRows;
	memalloc(NewRows, (newrowcount + 2) * sizeof(unsigned));
	for (unsigned i = 0; i < newridx; i++)
	{
		NewRows[i] = ctab->rows[i];
	}
	NewRows[newridx] = (newridx > 0 ? ctab->rows[newridx] : ctab->rows[0]);
	for (unsigned i = newridx + 1; i < newrowcount + 1; i++)
	{
		NewRows[i] = ctab->rows[i - 1] + rowsp;
	}
	NewRows[newrowcount + 1] = 0;
	srs(ctab->rows, NewRows);

	Conitem** NewCon;
	memalloc(NewCon, (newrowcount) * sizeof(struct Conitem*));
	MemCopyN((char*)NewCon, (const char*)ctab->cons, newridx * sizeof(struct Conitem*));
	NewCon[newridx] = citm;
	for (unsigned i = newridx + 1; i < newrowcount; i++)
	{
		NewCon[i] = ctab->cons[i - 1];
	}
	srs(ctab->cons, NewCon);
}

void CtabAppendColFore(struct Contab* ctab, struct Conitem* citm, unsigned colsp)
{
	if (!ctab || !citm) return;
	addr_t newcidx = ctab->curc;
	addr_t newcolcount = CtabColCount(ctab) + 1;
	addr_t newrowcount = CtabRowCount(ctab);
	unsigned* NewCols;
	memalloc(NewCols, (newcolcount + 2) * sizeof(unsigned));
	for (unsigned i = 0; i < newcidx; i++)
		NewCols[i] = ctab->cols[i];
	NewCols[newcidx] = (newcidx > 0 ? ctab->cols[newcidx] : ctab->cols[0]);
	for (unsigned i = newcidx + 1; i < newcolcount + 1; i++)
		NewCols[i] = ctab->cols[i - 1] + colsp;
	NewCols[newcolcount + 1] = 0;
	srs(ctab->cols, NewCols);

	for (addr_t i = 0; i < newrowcount; i++)
	{
		Conitem* NewCon;
		memalloc(NewCon, (newcolcount) * sizeof(struct Conitem));
		MemCopyN((char*)NewCon, (const char*)(ctab->cons[i]), newcidx * sizeof(struct Conitem));
		NewCon[newcidx] = citm[i];
		for (unsigned j = newcidx + 1; j < newcolcount; j++)
			NewCon[j] = ctab->cons[i][j - 1];
		srs(ctab->cons[i], NewCon);
	}
	memfree(citm);
	ctab->curc++;// point to the previous one
}

void CtabAppendColBack(struct Contab* ctab, struct Conitem* citm, unsigned colsp)
{
	// Based on the above.
	if (!ctab || !citm) return;
	addr_t newcidx = ctab->curc + 1;
	addr_t newcolcount = CtabColCount(ctab) + 1;
	addr_t newrowcount = CtabRowCount(ctab);
	unsigned* NewCols;
	memalloc(NewCols, (newcolcount + 2) * sizeof(unsigned));
	for (unsigned i = 0; i < newcidx; i++)
		NewCols[i] = ctab->cols[i];
	NewCols[newcidx] = (newcidx > 0 ? ctab->cols[newcidx] : ctab->cols[0]);
	for (unsigned i = newcidx + 1; i < newcolcount + 1; i++)
		NewCols[i] = ctab->cols[i - 1] + colsp;
	NewCols[newcolcount + 1] = 0;
	srs(ctab->cols, NewCols);

	for (addr_t i = 0; i < newrowcount; i++)
	{
		Conitem* NewCon;
		memalloc(NewCon, (newcolcount) * sizeof(struct Conitem));
		MemCopyN((char*)NewCon, (const char*)(ctab->cons[i]), newcidx * sizeof(struct Conitem));
		NewCon[newcidx] = citm[i];
		for (unsigned j = newcidx + 1; j < newcolcount; j++)
			NewCon[j] = ctab->cons[i][j - 1];
		srs(ctab->cons[i], NewCon);
	}
	memfree(citm);
}

void CtabRemoveRow(struct Contab* ctab)
{
	if (!ctab) return;
	addr_t newrowcount = CtabRowCount(ctab) - 1;
	if (newrowcount == 0) return;
	unsigned* NewRows;
	unsigned rowsp = ctab->rows[ctab->curr + 1] - ctab->rows[ctab->curr];
	memalloc(NewRows, (newrowcount + 2) * sizeof(unsigned));
	for (unsigned i = 0; i < ctab->curr; i++)
		NewRows[i] = ctab->rows[i];
	for (unsigned i = ctab->curr; i < newrowcount + 1; i++)
	{
		NewRows[i] = ctab->rows[i + 1] - rowsp;
	}
	NewRows[newrowcount + 1] = 0;
	srs(ctab->rows, NewRows);
	// Release the room.
	Conitem** NewCon;
	memalloc(NewCon, (newrowcount) * sizeof(struct Conitem*));
	MemCopyN((char*)NewCon, (const char*)ctab->cons, ctab->curr * sizeof(struct Conitem*));
	for (unsigned i = ctab->curr; i < newrowcount; i++)
	{
		NewCon[i] = ctab->cons[i + 1];
	}
	for (addr_t i = 0; i < CtabColCount(ctab); i++)
	{
		memfree(ctab->cons[ctab->curr][i].content);
	}
	memfree(ctab->cons[ctab->curr]);
	srs(ctab->cons, NewCon);
	if (ctab->curr)
		ctab->curr--;
}

void CtabRemoveCol(struct Contab* ctab)
{
	// Based on the above.
	if (!ctab) return;
	addr_t newcolcount = CtabColCount(ctab) - 1;
	addr_t newrowcount = CtabRowCount(ctab);
	if (newcolcount == 0) return;
	unsigned* NewCols;
	unsigned colsp = ctab->cols[ctab->curc + 1] - ctab->cols[ctab->curc];
	memalloc(NewCols, (newcolcount + 2) * sizeof(unsigned));
	for (unsigned i = 0; i < ctab->curc; i++)
		NewCols[i] = ctab->cols[i];
	for (unsigned i = ctab->curc; i < newcolcount + 1; i++)
		NewCols[i] = ctab->cols[i + 1] - colsp;
	NewCols[newcolcount + 1] = 0;
	srs(ctab->cols, NewCols);
	// Release the room.
	for (addr_t i = 0; i < newrowcount; i++)
	{
		Conitem* NewCon;
		memalloc(NewCon, (newcolcount) * sizeof(struct Conitem));
		MemCopyN((char*)NewCon, (const char*)(ctab->cons[i]), ctab->curc * sizeof(struct Conitem));
		for (unsigned j = ctab->curc; j < newcolcount; j++)
		{
			NewCon[j] = ctab->cons[i][j + 1];
		}
		memfree(ctab->cons[i][ctab->curc].content);
		srs(ctab->cons[i], NewCon);
	}
	if (ctab->curc)
		ctab->curc--;
}
#endif
