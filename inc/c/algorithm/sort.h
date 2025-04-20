// ASCII C TAB4 CRLF
// Docutitle: (Algorithm) Sorting
// Codifiers: @dosconio, @ArinaMgk
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

/* extend
sort SortTrait
find LocateTrait -> {FUTURE} SignedObject_T<idx> which has unwrap() and wrap()
*/

// Fit for ISO/IEC CPL Interfaces
#include "../stdinc.h"
#include "compare.h"

#define qsortof(arr,cmp) qsort(arr,numsof(arr),sizeof(*arr),cmp)

// CPL Style
#define cmpfof(iden) int iden(pureptr_t _a,pureptr_t _b)

#define cmpf_inhead(para_t,pa,pb) para_t *pa=(para_t*)_a, *pb=(para_t *)_b

//_TODO _ESYM_C void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));
//      _ESYM_C void* bsearch(const void* key, const void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*));

/*E*G*
cmpfof(cmp) {
	cmpf_inhead(int,pa,pb);
	return *pa - *pb;
}
*/

// We can print info in Compare_ft to see the order of traversal

//STYLE G-3
#if defined(_INC_CPP)
#include "../../cpp/trait/ArrayTrait.hpp"
#define setcmp(arr) Compare_ft cmp = (arr).Compare_f ? (arr).Compare_f : _Local_Compare
namespace uni {

	typedef _tocomp_ft Compare_ft;

	enum class SortMode {
		Bubble = 0, // O(n)     O(n^2)   O(n^2)              [KEEP-RELA]
		Insertion,  //                             === Bubble
		Selection,  // O(n^2)   O(n^2)   O(n^2)   [TIME-SAME]
		Heap,       // O(nlogn) O(nlogn) O(nlogn) [TIME-SAME][KEEP-RELA]
		Merge,      // O(nlogn) O(nlogn) O(nlogn) [TIME-SAME][KEEP-RELA][NEED-ALLOC(n)?]
		Quick,      // O(nlogn) O(nlogn) O(n^2)                         [NEED-ALLOC(nlogn)?]
		Shell,//{TODO} O(n) O(n^1.3) O(n^2)
	};

	void Sort(ArrayTrait& arr, SortMode sm = SortMode::Bubble);
}

#else 

#define setcmp(arr) _tocomp_ft cmp = (arr).func_comp ? (arr).func_comp : _Local_Compare


#endif

#endif
