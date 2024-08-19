// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant 
// LastCheck: RFZ22
// AllAuthor: @dosconio
// ModuTitle: Operations for ASCIZ Character-based String
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

#include "../../../../inc/c/ustring.h"
#include "../../../../inc/c/uctype.h"

static int sign = 0;

// decimal series

static void setsym(const char**const inp) {
	sign = 0;
	if (**inp == '-') sign++, (*inp)++;
	else if (**inp == '+') (*inp)++;
}

ptrdiff_t atoins(const char* str) // hosted-dep
{
	if (!str || !*str) return 0;
	setsym(&str);
	ptrdiff_t inst = 0;
	for0 (i, _size_decimal_get()) if (ascii_isdigit(str[i]))
	{
		inst *= 10;
		inst += (ptrdiff_t)ascii_digtoins(str[i]);
	} else break;
	return sign ? -inst : inst;
}
int _atoidefa(const char* inp) // hosted-dep
{
	if (!inp || !*inp) return 0;
	setsym(&inp);
	int inst = 0;
	for0(i, lookupDecimalDigits(sizeof(int))) if (ascii_isdigit(inp[i]))
	{
		inst *= 10;
		inst += (int)ascii_digtoins(inp[i]);
	} else break;
	return sign ? -inst : inst;
}

long int atolong(const char* inp) // hosted-dep
{
	if (!inp || !*inp) return 0;
	setsym(&inp);
	int inst = 0;
	for0(i, lookupDecimalDigits(sizeof(long))) if (ascii_isdigit(inp[i]))
	{
		inst *= 10;
		inst += (long)ascii_digtoins(inp[i]);
	} else break;
	return sign ? -inst : inst;
}


// atohex(inp 0~f)
// atobin(inp 0/1)

