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

#include "../../../../inc/c/aldbg.h"
#include "../../../../inc/c/ustring.h"

char* ChrFactorial(const char* a)
{
	char* res = StrHeap(aflaga.signsym ? "+1" : "1");
	if (a[1] == '0') goto endo;
	char* tmp = res;
	a = (const char*)StrHeap(a);
	while (ChrCmp(a, aflaga.signsym ? "+1" : "1") > 0)
	{
		srs(res, ChrMul(res, a));
		srs(a, ChrSub(a, aflaga.signsym ? "+1" : "1"));
	}
	memfree((char*)a);
endo:
	#ifndef _ARN_FLAG_DISABLE
	aflaga.overflow = 0;
	///malc_occupy = siz;
	aflaga.fail = 0;
	aflaga.sign = (*res == '-');
	#endif
	return res;
}
