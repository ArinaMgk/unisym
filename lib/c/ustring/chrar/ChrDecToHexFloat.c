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

// RFV3 (ArnMgk). E.g. "81" ---> "", based on ChrHexToDecFloat
//  0.99 --> FD70AF
//    0.99*16=15.84 0.84*16=13.44 0.44*16=7.04 0.04*16=0.64 0.64*16=10.24
//    0.24*64=15.36 0.36...
char* _Need_free ChrDecToHexFloat(const char* decf, size_t digits)
{
	#ifndef _ARN_FLAG_DISABLE
	struct _aflag_t tmpflg = aflaga;
	aflaga.signsym = 1;
	#endif
	size_t CrtLastDigs = StrLength(decf), CrtAfterDigs;
	char* const res = zalc(digits + 1);
	char* ptr = res;
	char* buf = malc(CrtLastDigs + 1 + 1 + 2);// Sign+Null+Extern
	buf[0] = '+';
	StrCopy(buf + 1, decf);
	for (size_t i = 0; i < digits; i++)
	{
		srs(buf, ChrMul("+16", buf));
		CrtAfterDigs = StrLength(buf + 1);
		if (CrtAfterDigs - CrtLastDigs == 2)
		{
			if (buf[1] == '1')
			{
				*ptr++ = 'A' + (buf[2] - '0');
			}
			#ifdef _dbg
			else (void)erro;
			#endif
			MemRelative(buf + 3, CrtAfterDigs - 2 + 1, -2);// include 0
		}
		else if (CrtAfterDigs - CrtLastDigs == 1)
		{
			*ptr++ = (buf[1]);
			MemRelative(buf + 2, CrtAfterDigs, -1);// include 0
		}
		else
		{
			*ptr++ = '0';
		}
	}
	memfree(buf);
	#ifndef _ARN_FLAG_DISABLE
	aflaga = tmpflg;
	#endif
	return res;
}
