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

//STYLE G-3
#if defined(_INC_CPP)
#include "../../cpp/trait/ArrayTrait.hpp"
#define setcmp(arr) Compare_ft cmp = (arr).Compare_f ? (arr).Compare_f : _Local_Compare
namespace uni {

	//typedef int (*Compare_ft)(pureptr_t a, pureptr_t b);
	typedef _tocomp_ft Compare_ft;

	enum SortMode {
		Bubble = 0, // for-for-if-xchg
		Selection, // for-for-if-logidx
		Insertion,
	};


	inline static void SortBubble(ArrayTrait& arr) {
		stduint count = arr.Length();
		setcmp(arr);
		for0(i, count) {
			int swapped = 0;
			for (stduint j = 0; j < count - i - 1; j++)
				if (cmp(arr.Locate(j), arr.Locate(j + 1)) > 0) {
					arr.Exchange(j, j + 1);
					swapped = 1;
				}
			if (!swapped) break;
		}
	}

	// Left area are ordered
	inline static void SortInsertion(ArrayTrait& arr) {
		stduint count = arr.Length();
		setcmp(arr);
		for1(i, count - 1) {
			stdsint j = i - 1;
			while (j >= 0 && cmp(arr.Locate(j), arr.Locate(j + 1)) > 0) {
				arr.Exchange(j, j + 1);
				j--;
			}
		}
	}
	
	// Left area are ordered
	inline static void SortSelection(ArrayTrait& arr) {
		stduint count = arr.Length();
		setcmp(arr);
		for0(i, count - 1) {
			stduint clim_one = i;
			for (stduint j = i + 1; j < count; j++) if (cmp(arr.Locate(clim_one), arr.Locate(j)) > 0)
			clim_one = j;
			arr.Exchange(i, clim_one);
		}
	}

	inline static void Sort(ArrayTrait& arr, SortMode sm = SortMode::Bubble) {
		void (*sort_f)(ArrayTrait & arr) = nullptr;
		switch (sm) {
		case SortMode::Bubble: sort_f = SortBubble; break;
		case SortMode::Selection: sort_f = SortSelection; break;
		case SortMode::Insertion: sort_f = SortInsertion; break;
		default: sort_f = nullptr; break;
		}
		asserv(sort_f)(arr);
	}
}

#else 

#define setcmp(arr) _tocomp_ft cmp = (arr).func_comp ? (arr).func_comp : _Local_Compare


#endif

#endif
