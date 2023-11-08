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

#include "../stack.h"
#include "../aldbg.h"
#include "../ustring.h"
#include <stdlib.h>

static struct stack_t glostack;

// always:
// - baseptr <= current
// - stack top : baseptr + size >= current

int stack_init(struct stack_t* stack, size_t size)
{
	if(!stack) stack = &glostack;
	if(stack->baseptr) stack_free(stack);
	///size = (size + 1) & ~(size_t)1;// align word
	stack->baseptr = (byte*)malc(size);
	if (stack->baseptr == NULL)
		return 1;
	stack->pointer = stack->baseptr + size;
	stack->size = size;
	return 0;
}

int stack_push(struct stack_t* stack, const void* data, size_t size)
{
	if(!stack) stack = &glostack;
	if (stack->baseptr == NULL || !size)
		return 1;
	if (stack->baseptr <= stack->pointer - size && stack->baseptr + stack->size >= stack->pointer)
	{
		stack->pointer -= size;
		MemCopyN(stack->pointer, data, size);
		if(stack->baseptr > stack->pointer - size)// cannot contain the next one
			callif(stack->if_full);
		return 0;
	}
	else return 1;
}

int stack_pop(struct stack_t* stack, void* data, size_t size)
{
	if(!stack) stack = &glostack;
	if (stack->baseptr == NULL || !size)
		return 1;
	if (stack->baseptr <= stack->pointer && stack->baseptr + stack->size >= stack->pointer + size)
	{
		MemCopyN(data, stack->pointer, size);
		stack->pointer += size;
		return 0;
	}
	else return 1;
}

void stack_free(struct stack_t* stack)
{
	if(!stack) stack = &glostack;
	if (stack->baseptr != NULL)
	{
		memf(stack->baseptr);
		stack->baseptr = NULL;
		stack->pointer = NULL;
	}
}

void stack_global_event_full(void (*if_full)(void))
{
	glostack.if_full = if_full;
}
