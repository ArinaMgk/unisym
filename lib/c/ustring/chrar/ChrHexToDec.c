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

char* _Need_free ChrHexToDec(const char* hex)// slv using
{
	int sign = 0;
	int Signed
	#ifndef _ARN_FLAG_DISABLE
		= aflaga.signsym;
	#else
		=1;	
	#endif
	///if (!hex || !*hex)
	if (*hex == '-')sign++, hex++;
	else if (*hex == '+')hex++;
	size_t hexlen = StrLength(hex);// skip sign digit
	
	char* sum, c;
	sum = StrHeap(Signed ? (sign ? "-0" : "+0") : "0");
	//
	for (size_t i = 0; i < hexlen; i++)// loop
	{
		c = hex[i];
		char* ttemp = StrHeap(Signed ? "+00" : "00");// 0~F 0~15
		char* ttemp2 = ChrInsPow(Signed ? "+16" : "16", hexlen - i - 1);
		if (c <= '9' && c >= '0') ttemp[2 - (!Signed)] += c - '0';
		else
		{
			ttemp[2 - (!Signed)] += c - (c >= 'a' ? 'a' : 'A');
			ttemp[1 - (!Signed)] = '1';
		}
		srs(ttemp, ChrMul(ttemp, ttemp2));
		srs(sum, ChrAdd(sum, ttemp));
		memfree(ttemp);
		memfree(ttemp2);
	}
	if (sign && Signed && *sum == '+') *sum = '-';
	#ifndef _ARN_FLAG_DISABLE
	///malc_occupy = StrLength(ptr_tempmas) + 1;
	aflaga.overflow = 0;
	aflaga.fail = 0;
	aflaga.sign = sign;
	#endif
	return sum;
}
