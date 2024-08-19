// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ23
// AllAuthor: @dosconio
// ModuTitle: TENSOR ARITHMETIC and OPERATION(for non-arith-elements)
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

#include "../../inc/c/stdinc.h"
#include "../../inc/c/tenar.h"

/// /// /// {TODO CHECK} /// /// ///

// param:dimen : will never equal to 0.
static size_t TenNewSub(struct tenelm* crtte, size_t* dimen, void* (*InitFunc)(size_t idnumber))
{
	crtte->count = dimen[0];
	if(dimen[1]) crtte->dimen = TenNewSub((struct tenelm*)crtte->datas, dimen + 1, InitFunc);// while (dimen[crtte->dimen]) crtte->dimen++;
	else crtte->dimen = 1;
	crtte->datas = zalc(sizeof(void*) * crtte->count);
	for (size_t i = 0; i < crtte->count ; i++)
		if (InitFunc)
			crtte->datas[i] = InitFunc(i);
	return crtte->dimen;
}


// param:dimen[a string ends with 0, e.g. {3,2,1,0} means 3*2*1(tensor), {3,2,0} means 3*2(matrix), {3,0} means 3(vector), {0} means 0(scalar)] \
	param:InitFunc[0 for zalloc, else e.g. for {a<3,b<2,c=0} of {3,2,1,0}, the idnumber will be a*(2*1)+b*1+c, and the InitFunc will be called with the idnumber]
tensor* TenNew(size_t type, size_t* dimen, void* (*InitFunc)(size_t idnumber))
{
	tensor* ret = malcof(tensor);
	ret->type = type;
	if(*dimen) TenNewSub(&ret->data, dimen, InitFunc);
	return ret;
}

static void TenPrintSub(const struct tenelm* crtte, void (*PrintFunc)(const char* str))
{
	if (!crtte || !PrintFunc) return;
	PrintFunc("[ ");
	for (size_t i = 0; i < crtte->count; i++)
	{
		// {TODO review}
		if (crtte->dimen) TenPrintSub(crtte->datas[i], PrintFunc);
		else PrintFunc(crtte->datas[i]);
		PrintFunc(" ");
	}
	PrintFunc("]");
}

void TenPrint(const tensor* ten, void (*PrintFunc)(const char* str))
{
	if (!ten || !PrintFunc) return;
	TenPrintSub(&ten->data, PrintFunc);
}
