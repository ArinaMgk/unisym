// ASCII TAB4 C99 ArnCovenant TAB CRLF
// @dosconio
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

// hard stack, whose size of elements should be times of that of word
// soft stack, whose size of elements should be times of that of byte
// - global stack

#ifndef _LIB_STACK
#define _LIB_STACK

#include "alice.h"

#define STACK_GLOBAL 0

struct stack_t
{
	byte* baseptr;
	byte* pointer;
	size_t size;
	void (*if_full)(void);
	unsigned stack_direction : 1;// {unused} 0: dec(default), 1: inc
};// by @dosconio

typedef struct stack_t stack_t;

#define push(x) stack_push(0, &(x), sizeof(x))
#define pop(x) stack_pop(0, (void*)&(x), sizeof(x))

// Return 0 if success, else return -1

int stack_init(struct stack_t* stack, size_t size);

int stack_push(struct stack_t* stack, const void* data, size_t size);

int stack_pop(struct stack_t* stack, void* data, size_t size);

void stack_free(struct stack_t* stack);

void stack_global_event_full(void (*if_full)(void));

#ifdef _AUTO_INCLUDE
	#include "./source/stack.c"
#endif // _AUTO_INCLUDE

#endif // !_LIB_STACK
