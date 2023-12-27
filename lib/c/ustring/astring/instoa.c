// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ22
// AllAuthor: @dosconio
// ModuTitle: Operations for ASCIZ Character-based String Specially output heap
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

char* instoa(ptrdiff_t num)
{
	#ifndef _ARN_FLAG_DISABLE
	size_t numlen = aflaga.signsym + 1;
	#else
	size_t numlen = 2;
	#endif
	size_t numslv = num > 0 ? num : -num;
	AddDecimalDigitsLen(numlen, numslv);
	#ifndef _ARN_FLAG_DISABLE
	if (numlen > malc_limit)
	{
		///malc_occupy = 0;
		aflaga.fail = 1;
		call_state = 1;
		return 0;
	}
	#endif
	//
	char* buf = salc(numlen);
	char* p = buf + numlen - 2;
	#ifndef _ARN_FLAG_DISABLE
	if (aflaga.signsym)
	#endif
		*buf = num < 0 ? '-' : '+';
	numslv = num > 0 ? num : -num;
	do
	{
		*p-- = (numslv % 10) + '0';
		numslv /= 10;
	} while (numslv);
	#ifndef _ARN_FLAG_DISABLE
	aflaga.overflow = 0;
	aflaga.fail = 0;
	aflaga.sign = num < 0;
	///malc_occupy = numlen;
	#endif
	return buf;
}
