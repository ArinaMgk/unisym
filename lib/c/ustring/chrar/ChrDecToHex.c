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

char* ChrDecToHex(char* dec)// Output: upper case
{
	// pass a non-0~9 number will cause potential mistake.
	int sign = 0;
	int Signed
	#ifndef _ARN_FLAG_DISABLE
		= aflaga.signsym;
	#else
		=1;	
	#endif
	///if (!dec || !*dec)
	if(Signed)
	if (*dec == '-')sign++, dec++;
	else if (*dec == '+') dec++;
	const char* list = { "0123456789ABCDEF" };
	size_t declen = StrLength(dec);
	size_t declen2 = declen;
	char diver[4], * p;
	#ifndef _ARN_FLAG_DISABLE
	if (declen + 1 + 1 > malc_limit)
	{
		///malc_occupy = 0;
		aflaga.fail = 1;
		call_state = 1;
		return 0;
	}
	#endif
	p = malc(declen + 1 + 1);
	if(Signed) *p = '+';
	// here
	StrCopy(p + !(!Signed), dec);
	while (declen)
	{
		StrCopy(diver, Signed ? "+16" : "16");
		ChrDiv(p, diver);
		if (diver[2-!Signed]) dec[declen - 1] = "ABCDEF"[diver[2-!Signed] - '0'];
		else dec[declen - 1] = diver[1-!Signed];
		declen--;
	}
	ChrCpz(dec);
	if (Signed) dec[-1] = sign ? '-' : '+';
	memfree(p);
	#ifndef _ARN_FLAG_DISABLE
	///malc_occupy = declen2 + 1 + 1;
	aflaga.overflow = 0;
	aflaga.fail = 0;
	aflaga.sign = sign;
	#endif
	return dec - Signed;
}
