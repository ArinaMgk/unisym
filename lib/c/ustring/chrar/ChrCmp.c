// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant 
// LastCheck: RFZ22
// AllAuthor: @dosconio
// ModuTitle: Operations for ChrAr
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

int ChrCmp(const char* a, const char* b)
{
	int sga = 0, sgb = 0;
	const char* endofa = a, * endofb = b;
	int greater = 1;
	if (!StrCompare(a, b)) return 0;
	for (; *endofa; endofa++); endofa--;
	for (; *endofb; endofb++); endofb--;
	if (*a == '+')a++;
	if (*b == '+')b++;
	if (*a == '-') { a++; sga = 1; }
	if (*b == '-') { b++, sgb = 1; }
	if (*a == '0' && *b == '0') return 0;// more effective
	if (endofa - a == endofb - b) greater = (StrCompare(a, b)) > 0;
	else greater = endofa - a > endofb - b;
	if (sga ^ sgb) return sga ? -1 : 1;
	return ((!sga) ^ (!greater)) ? 1 : -1;
}
