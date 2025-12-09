// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Format.Data) GNU tar
// Codifiers: @ArinaMgk
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

#ifndef _INC_HeaderTemplate_X
#define _INC_HeaderTemplate_X

#include "../../stdinc.h"

#if defined(_INC_CPP)
namespace uni {
#endif

	// Borrowed from GNU-tar
	struct data_tar_header
	{
		char name[100];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char chksum[8];
		char typeflag;
		char linkname[100];
		char magic[6];
		char version[2];
		char uname[32];
		char gname[32];
		char devmajor[8];
		char devminor[8];
		char prefix[155];
		/* 500 */
	};



#if defined(_INC_CPP)
}
#endif
#endif
