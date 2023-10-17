// ASCII TAB4 C99 ArnAssume
// Integer
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include <stddef.h>


//{TEMP} no user-defined judging function


// para:direction [0:ascend 1:descend]
#define IntBubbleSort(array, size, direction) \
	for (size_t i = 0; i < (size) - 1; i++) \
		for(size_t j = i + 1; j < (size); j++) \
			if ((array[i] > array[j]) ^ (direction)) \
				array[i] ^= array[j] ^= array[i] ^= array[j];
// E.g. IntBubbleSort(my_ints, numsof(my_ints), 0);

