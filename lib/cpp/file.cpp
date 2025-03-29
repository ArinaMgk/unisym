// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: File
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
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

#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <stdlib.h>

//{TEMP} avoid conflict with stdlib.h
#define _INC_STDLIB

#include "../../inc/c/file.h"

namespace uni {
#ifdef _WinNT
	HostFile::~HostFile() {
		if (!fptr) return;
		Letvar(p, FILE*, fptr);
		fclose(p);
		fptr = nullptr;
	}

	HostFile::HostFile(rostr filepath, FileOpenType fopen_type) {
		static rostr fop_iden[]{
			"r", "w", "a"
		};
		fptr = (void*)fopen(filepath, fop_iden[_IMM(fopen_type)]);
	}

	bool HostFile::operator>> (byte& B) {
		int ch = fgetc((FILE*)fptr);
		B = (ch != EOF) ? ch : 0;
		return (ch != EOF);
	}

	bool HostFile::operator>> (String& str) {
		byte B;
		bool state;
		while (str.available()) {
			state = self >> B;
			if (B || str.getTermicharCount())
				str << B;
			if (!state) break;
		}
		return true;
	}
#endif
};
