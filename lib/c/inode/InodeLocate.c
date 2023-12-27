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
#include "../../../inc/c/ustring.h"
#include "../../../inc/c/inode.h"

inode* InodeLocate(inode* inp, const char* iden, inode** refleft)
{
	inode* crt = inp;
	if (refleft) *refleft = 0;
	if (!inp) return 0;
	do if (crt->addr && !StrCompare(iden, crt->addr))
		return crt;
	while ((!refleft || (*refleft = crt)) && (crt = crt->next));
	return 0;
}
