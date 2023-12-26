// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ21
// AllAuthor: @dosconio
// ModuTitle: Identification Node
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

#include "../../../inc/c/aldbg.h"
#include "../../../inc/c/inode.h"
#include "../../../inc/c/ustring.h"

// prop: [... |keep_type|readonly]
inode* InodeUpdate(inode* inp, const char* iden, void* data, size_t typ, size_t prop, void(*freefunc_element)(void*))
{
	inode* left = 0;
	inode* crt = 0;
	inode info = { .data = data };// {TODO} make use of this for memset dest
	if (!inp)
		crt = zalcof(inode);
	else if (crt = InodeLocate(inp, iden, &left))
	{
		if ((crt->property & INODE_READONLY) && prop == 0x80)
		{
			//{ERRO} Change the readonly object.
			if (freefunc_element && data) freefunc_element(&info);
			return 0;
		}
		else if ((crt->property & INODE_TYPEKEEP) && prop == 0x80)
		{
			//{ERRO} Change the special object into invalid type.
			if (freefunc_element && data) freefunc_element(&info);
			return 0;
		}
		if (freefunc_element) freefunc_element(crt); else { memf(crt->data);/* addr? */ }
	}
	else
		crt = left->next = zalcof(inode);// as the last one and assert(left)
	if (!crt->addr) crt->addr = StrHeap(iden);
	crt->property = prop & 0b11;
	crt->data = data;
	crt->type = typ;
	return crt;
}
