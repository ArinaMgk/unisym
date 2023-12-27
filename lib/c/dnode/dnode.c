// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ21
// AllAuthor: @ArinaMgk(till RFA03) @dosconio
// ModuTitle: Double-directions Node
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

// AFLAGA {ASF, DF, FF, ZF, ONF}
// - ASF : Auto sort the dnode by the DF
// - DF  : in increasing or decreasing order
// - FF  : something wrong raised
// - ZF  : zero flag, if the dnode is empty
// - ONF : order flag, if the dnode has and only has one dnode

#include "../../../inc/c/dnode.h"
#include "../../../inc/c/aldbg.h"

static const char _USYM_IDEN_[] = {
	"UNISYM under Apache 2.0 Licence @ArinaMgk, @dosconio"
};

int (*_dnode_compare)(void* addr0, void* addr1);

int _dnode_freepass = 0;

// used signature: void DnodeReleaseTofreeDefault(void* inp);
static void DnodeReleaseSingle(void* inp)
{
	if (_dnode_freepass)
		memf(((dnode*)inp)->addr);
	// else just pass addr, meaning the addr may have the heap memory
	memf(inp);
} void(*_dnode_freefunc)(void*) = DnodeReleaseSingle;

dnode* _dnode_first;


