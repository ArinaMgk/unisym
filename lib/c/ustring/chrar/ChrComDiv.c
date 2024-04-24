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

char* ChrComDiv(const char* op1, const char* op2)
{
	//1. Get the below one, assume it as op1
	//2. !(op2%op1) then dec a;
	// to polish
	if (op2[!!aflaga.signsym] == '0') return StrHeap(op2);
	if (aflaga.signsym && (*op1 != '+' && *op1 != '-')) { op1 = StrHeapAppend("+", op1); }
	else op1 = StrHeap(op1);
	if (op1[!!aflaga.signsym] == '0') return (char*)op1;
	if (aflaga.signsym && (*op2 != '+' && *op2 != '-')) { op2 = StrHeapAppend("+", op2); }
	else op2 = StrHeap(op2);
	int state = ChrCmp(op1, op2);
	if (!state)// equal
	{
		memfree(op2);
		return (char*)op1;
	}
	if (state < 0) xchgptr(op1, op2);
	// now +op1 > +op2
	char* tmp_op1;
	while (op2[!!aflaga.signsym] != '0')
	{
		tmp_op1 = StrHeap(op1);
		srs(op1, StrHeap(op2));
		ChrDiv(tmp_op1, (char*)op2);
		memfree(tmp_op1);
	}
	memfree(op2);
	return (char*)op1;
}
