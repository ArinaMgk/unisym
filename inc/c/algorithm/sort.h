// ASCII C TAB4 CRLF
// Docutitle: (Algorithm) Sorting
// Codifiers: @dosconio: 20240602
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
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

#ifndef _INC_Sort
#define _INC_Sort

// Fit for ISO/IEC CPL Interfaces

#define qsortof(arr,cmp) qsort(arr,numsof(arr),sizeof(*arr),cmp)

// CPL Style
#define cmpfof(iden,para_t) int iden(pureptr_t _a,pureptr_t _b)

#define cmpf_inhead(para_t) para_t *pa=(para_t*)_a, *pb=(para_t *)_b


/*E*G*
cmpfof(cmp,int) {
	cmpf_inhead(int);
	return *pa - *pb;
}
*/

#endif
