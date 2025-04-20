// ASCII C TAB4 CRLF
// Docutitle: (Algorithm) Sorting
// Codifiers: @ArinaMgk
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

#include "../../inc/c/algorithm/sort.h"

namespace uni {

	static void SortBubble(ArrayTrait& arr) {
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
	static void SortInsertion(ArrayTrait& arr) {
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
	static void SortSelection(ArrayTrait& arr) {
		stduint count = arr.Length();
		setcmp(arr);
		for0(i, count - 1) {
			stduint clim_one = i;
			for (stduint j = i + 1; j < count; j++) if (cmp(arr.Locate(clim_one), arr.Locate(j)) > 0)
				clim_one = j;
			arr.Exchange(i, clim_one);
		}
	}

	static void SortBintreeAdapt(ArrayTrait& arr, stduint index, stduint count, _tocomp_ft& cmp) {
		if (count < 2) return;
		if (2 * index + 1 < count) {
			SortBintreeAdapt(arr, 2 * index + 1, count, cmp);
			if (cmp(arr.Locate(index), arr.Locate(2 * index + 1)) < 0)
				arr.Exchange(index, 2 * index + 1);
		}
		if (2 * index + 2 < count) {
			SortBintreeAdapt(arr, 2 * index + 2, count, cmp);
			if (cmp(arr.Locate(index), arr.Locate(2 * index + 2)) < 0)
				arr.Exchange(index, 2 * index + 2);
		}
	}
	static void SortBintree(ArrayTrait& arr) {
		stduint count = arr.Length();
		setcmp(arr);
		if (count < 2) return;
		// Select bigger in the left
		for1r(i, count - 1) {// index yo [1, count - 1]
			stduint index_parent = (i - 1) / 2;
			if (cmp(arr.Locate(index_parent), arr.Locate(i)) < 0)
				arr.Exchange(i, index_parent);
		}
		// Sort the leftest on the right
		while (count) {
			arr.Exchange(0, count-- - 1);
			SortBintreeAdapt(arr, 0, count, cmp);
		}
	}

	// ArinaMgk Method: no using any extra memory
	static void SortMerge_MergeArn(ArrayTrait& arr, stduint left, stduint lens, _tocomp_ft& cmp) {
		if (lens < 2) return;
		stduint pleft = left, pright = left + lens / 2;
		stduint lleft = lens / 2, lright = lens - lens / 2;
		while (lleft && lright) {
			if (cmp(arr.Locate(pleft), arr.Locate(pright)) <= 0) lleft--;
			else {
				//   |     |
				// 1 4 5 , 2 3
				for0(i, lleft) arr.Exchange(pright, pleft + i);
				pright++, lright--;
				//     |   |
				// 1 2 4 5 3
			}
			pleft++;
		}
	}
	static void SortMergeSub(ArrayTrait& arr, stduint left, stduint lens, _tocomp_ft& cmp) {
		if (lens < 2) return;
		SortMergeSub(arr, left, lens / 2, cmp);
		SortMergeSub(arr, left + lens / 2, lens - lens / 2, cmp);
		SortMerge_MergeArn(arr, left, lens, cmp);
	}
	static void SortMerge(ArrayTrait& arr) {
		stduint count = arr.Length();
		setcmp(arr);
		SortMergeSub(arr, 0, count, cmp);
	}

	// ArinaMgk Method: no using any extra memory
	static void SortQuickSub(ArrayTrait& arr, stduint left, stduint lens, _tocomp_ft& cmp) {
		if (lens < 2) return;
		stduint head = left, tail = left + lens - 1;
		stduint key = (head);
		while (head < tail) {
			while (head < tail && cmp(arr.Locate(key), arr.Locate(tail)) <= 0) tail--;
			if (head < tail) {
				arr.Exchange(key, tail);
				key = tail;
			}
			while (head < tail && cmp(arr.Locate(head), arr.Locate(key)) <= 0) head++;
			if (head < tail) {
				arr.Exchange(key, head);
				key = head;
			}
		}
		SortQuickSub(arr, left, key - left, cmp);
		SortQuickSub(arr, key + 1, lens - key + left - 1, cmp);
	}
	static void SortQuick(ArrayTrait& arr) {
		stduint count = arr.Length();
		setcmp(arr);
		SortQuickSub(arr, 0, count, cmp);
	}




	static void (*sortfuns[])(ArrayTrait& arr) {
		SortBubble,
		SortInsertion,
		SortSelection,
		SortBintree,
		SortMerge,
		SortQuick,
	};
	void Sort(ArrayTrait& arr, SortMode sm) {
		if (_IMM(sm) < numsof(sortfuns)) {
			sortfuns[_IMM(sm)](arr);
		}
	}

}

