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

// RFV3 (ArnMgk). E.g. "FE" stands for "0.FEH" and this return "9921875" stand for "0.9921875"
//   (15/16+14/256) or (254/256), so "ABCD" : "ABH/256+CDH/256/256"
//   171_0_0000_0000_0000_0000/256 = 66796875000000000
//   205_0_0000_0000_0000_0000/65536=  312805175781250
//   then cut the trailing zeros and the sign prefix{TODO optional}
// ! input upper cases and this excludes element check.
char* _Need_free ChrHexToDecFloat(const char* hexf)
{
	// assume limit 0X800, adapt in the future.
	#ifndef _ARN_FLAG_DISABLE
	struct _aflag_t tmpflg = aflaga;
	aflaga.signsym = 1;
	#endif
	char unit_diver[] = "+16";
	char* CrtDiver = StrHeap(unit_diver);// keep this size < 0x1000
	char* temp = zalc(0X800 + 1);
	char* temp_for_crt = zalc(0X800 + 1);
	size_t tempexpo = (size_t)0X800 - 3;// based on 10
	char* res = StrHeap("+0");
	size_t paralen = StrLength(hexf);
	for (size_t i = 0; i < paralen; i++)
	{
		char* ptr = temp;
		*ptr++ = '+';
		if (hexf[i] >= '0' && hexf[i] <= '9') *ptr++ = hexf[i];
		else
		{
			*ptr++ = '1';
			*ptr++ = hexf[i] - 'A' + '0';
		}
		size_t CrtCompensate = tempexpo;// 0x1000 - (ptr - temp);
		MemSet(ptr, '0', CrtCompensate);
		ptr[CrtCompensate] = 0;
		char* crtdiv = StrCopyN(temp_for_crt, CrtDiver, 0X800);
		ChrDiv(temp, crtdiv);// LIMIT EXIS
		srs(res, ChrAdd(res, temp));
		srs(CrtDiver, ChrMul(CrtDiver, unit_diver));
	}
	memfree(CrtDiver);
	memfree(temp);
	memfree(temp_for_crt);
	ChrCtz(res);
	srs(res, StrHeap(res + 1));
	#ifndef _ARN_FLAG_DISABLE
	aflaga = tmpflg;
	#endif
	return res;
}
