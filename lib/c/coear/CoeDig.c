/// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ23
// AllAuthor: @ArinaMgk
// ModuTitle: ASCII Powerful Number of Arinae
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

#include "../../../inc/c/coear.h"

void CofIncDig(coe* obj, size_t digits)
{
	unsigned char sgn_digit = obj->coff[0];
	DigInc('1', obj->coff + digits);
	// E.g. "+123456???"
	if (obj->coff[0] != sgn_digit)
	{
		/// char tmpbuf[2] = {0};
		/// *tmpbuf = '0' + obj->coff[0] - sgn_digit;
		MemRelative(obj->coff + 1, digits, 1);
		srs(obj->expo, ChrAdd(obj->expo, "+1"));
		/// srs(obj->coff, StrHeapInsertThrow(obj->coff, tmpbuf, 1, 0));
		obj->coff[1] = '0' + obj->coff[0] - sgn_digit;
		obj->coff[0] = sgn_digit;
	}
}

void CoeDig(coe* obj, size_t digits, int direction)
{
	if (obj->divr[1] == '0') return;
	if (!digits) digits = show_precise;
	size_t CrtPrecise = 0;
	while (obj->coff[CrtPrecise + 1]) CrtPrecise++;
	if (digits == CrtPrecise) return;
	else if (CrtPrecise > digits)// Cut. CrtPrecise is at least 2, which greater than 9.
	{
		//+123 CrtPrecise=3
		//0123
		// Appended RFW22:16:40
		int rest_al_zero = 1;
		{
			size_t tmplen = 1;
			char tmpc;
			while (tmpc = obj->coff[digits + tmplen])
			{
				if (tmpc != '0')
				{
					rest_al_zero = 0;
					break;
				}
				tmplen++;
			}
		}
		// diection:0: need not anything
		if (direction == 2 // Nearest, e.g. 1.5 to 2
			&& obj->coff[digits + 1] >= '5') CofIncDig(obj, digits);
		else if (!rest_al_zero)
		{
			size_t coflen = 0;
			while (obj->coff[coflen])coflen++;
			if (direction == 1)// to +inf
				if (*obj->coff == '+')
					CofIncDig(obj, digits);
				else;// '-'
			else if (direction == 3)// to -inf
				if (*obj->coff == '+')
					;
				else CofIncDig(obj, digits);
			else if (direction == 4)// to out
				CofIncDig(obj, digits);
		}
		obj->coff[digits + 1] = 0;
		char* internum;
		/// for (CrtPrecise = 0; obj->coff[CrtPrecise + 1]; CrtPrecise++);
		srs(obj->expo, ChrAdd(obj->expo, internum = instoa(CrtPrecise - digits)));
		memfree(internum);
	}
	// Here are RFW21:11:35 appended for adding suffixed zero.
	else
	{
		// the diff zo (digits-CrtPrecise)
		char* tmp = obj->coff;
		size_t tmplen = StrLength(tmp);
		obj->coff = (char*)malloc(1 + tmplen + (digits - CrtPrecise));
		StrCopy(obj->coff, tmp);
		obj->coff[tmplen + (digits - CrtPrecise)] = 0;
		MemSet(obj->coff + tmplen, '0', digits - CrtPrecise);
		free(tmp);
		tmp = instoa(digits - CrtPrecise);
		srs(obj->expo, ChrSub(obj->expo, tmp));
		memfree(tmp);
	}
}
