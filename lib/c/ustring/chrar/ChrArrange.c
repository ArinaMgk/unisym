// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
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

char* ChrArrange(const char* total, const char* items)// Amn
{
	int Signed
	#ifndef _ARN_FLAG_DISABLE
		= aflaga.signsym;
	#else
		=1;	
	#endif
	char* res = StrHeap(Signed ? "+1" : "1");
	items = StrHeap(items);
	total = StrHeap(total);
	while (ChrCmp(items, "+0") > 0)
	{
		srs(res, ChrMul(res, total));
		srs(total, ChrSub(total, Signed ? "+1" : "1"));
		srs(items, ChrSub(items, Signed ? "+1" : "1"));
	}
	memfree(items);
	memfree(total);
	#ifndef _ARN_FLAG_DISABLE
	aflaga.overflow = 0;
	///malc_occupy = siz;
	aflaga.fail = 0;
	aflaga.sign = (*res == '-');
	#endif
	return res;
}
