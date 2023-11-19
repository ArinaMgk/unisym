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

struct tenelm
{
	size_t count;
	size_t dimen;// if not zero, data points to next tenelm
	void** datas;
};

typedef struct tensor
{
	size_t type;
	struct tenelm data;
} tensor;

/* Nested structure and for example 3*2*0 {
	row0 {col0{} col1{}}
	row1 {col0{} col1{}}
	row2 {col0{} col1{}}
	}
Arina Design: end by 0, e.g. 2*2*2(tensor), 3(vector), 2*2(matrix), 0(scalar)
*/







#endif
