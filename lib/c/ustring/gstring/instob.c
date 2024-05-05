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

char* instob(ptrdiff_t num, char* buf)
{
	#ifdef _ARN_FLAG_ENABLE
	size_t numlen = aflag.Signed + 1;
	#else
	size_t numlen = 2;
	#endif
	size_t numslv = num > 0 ? num : -num;
	movDecimalDigitsLen(numlen, numslv);
	#ifdef _ARN_FLAG_ENABLE
	if (malc_limit < numlen || malc_limit < (unsigned)3 - !arna_eflag.Signed || !buf)
	{
		///malc_occupy = 0;
		arna_eflag.Failure = 1;
		call_state = 1;
		return 0;
	}
	#endif
	//
	char* p = buf + numlen - 2;
	#ifdef _ARN_FLAG_ENABLE
	if (aflag.Signed)
	#endif
		*buf = num < 0 ? '-' : '+';
	buf[numlen - 1] = 0;
	numslv = num > 0 ? num : -num;
	do
	{
		*p-- = (numslv % 10) + '0';
		numslv /= 10;
	} while (numslv);
	#ifdef _ARN_FLAG_ENABLE// {TOFIX}
	aflag.PrecLoss = 0;
	aflag.Failure = 0;
	aflag.Sign = num < 0;
	///malc_occupy = numlen;
	#endif
	return buf;
}
