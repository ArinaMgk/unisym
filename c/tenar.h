// ASCII TAB4 C99 ArnAssume
// TENSOR ARITHMETIC and OPERATION(for non-arith-elements)

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

#ifndef _LIB_TENSOR
#define _LIB_TENSOR
#include <stddef.h>
// parallel with ustring::nnode
typedef struct tensor
{
	size_t* dimen;
	union { void* first; struct tensor* tenfirst; };
} tensor;
/* Nested structure and for example 3*2*0 {
	ln0 {2 col{} {}}
	ln1 {2 col{} {}}
	ln2 {2 col{} {}}
	}
Arina Design: end by 0, e.g. 2*2*2(tensor), 3(vector), 2*2(matrix), 0(scalar)
*/

#endif
