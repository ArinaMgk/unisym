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

char* ChrComMul(const char* op1, const char* op2)
{
	// assume the GCD is g
	// then LCM = m*(b/m)*(a/m) = a*b/m
	// to polish
	char* res = ChrMul(op1, op2);
	char* m = ChrComDiv(op1, op2);
	ChrDiv(res, m);// quo=>res rem=>m
	memfree(m);
	return res;
}
