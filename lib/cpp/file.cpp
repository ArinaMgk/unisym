// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: File
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Dependens: stdc(++)
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
#if defined(_WinNT) || defined(_Linux)
	HostFile::~HostFile() {
		if (!fptr) return;
		Letvar(p, FILE*, fptr);
		fclose(p);
		fptr = nullptr;
	}

	HostFile::HostFile(rostr filepath, FileOpenType fopen_type) {
		static rostr fop_iden[]{
			"rb", "wb", "ab"
		};
		fptr = (void*)fopen(filepath, fop_iden[_IMM(fopen_type)]);
	}

	//

	int HostFile::inn() {
		return fgetc((FILE*)fptr);
	}
	int HostFile::out(const char* str, stduint len) {
		return _TODO - 1;
	}

	//

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

	bool HostFile::operator<< (byte B) {
		int ch = fputc(B, (FILE*)fptr);
		return (ch != EOF);
	}

	//

	bool HostFile::FetchLine(char* buf, stduint len) {
		bool state = fgets(buf, len, (FILE*)fptr);
		for (stduint i = 0; buf[i]; i++) {
			if (buf[i] == '\n') {
				buf[i + 1] = '\0';
				break;
			}
			if (buf[i] == '\r') {
				buf[i] = '\n';
				buf[i + 1] = '\0';
				break;
			}
		}// \n \r \n\r \r\n -> \n
		return state;
	}



	//

#endif
};
