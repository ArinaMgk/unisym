// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant CPU(80586+)
// LastCheck: RFZ03
// AllAuthor: @dosconio
// ModuTitle: Generator for table of toupper and tolower
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

//{TEMP} Rely on other implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main()
{
	const char* ulibpath = getenv("ulibpath");
	char dest_filename[128];
	const char copyright_info[] = "http://unisym.org/license.html";

	strcpy(dest_filename, ulibpath);
	strcat(dest_filename, "/c/auxiliary/toxxxer.c");
	FILE* dest_file = fopen(dest_filename, "w");
	if (dest_file == NULL) {
		printf("Error: Cannot open file %s\n", dest_filename);
		return 1;
	}
	fprintf(dest_file, "// ASCII C99 TAB4 CRLF\n");
	fprintf(dest_file, "// Attribute: Origin(toxxxer.make.c)\n");
	fprintf(dest_file, "// AllAuthor: @dosconio\n");
	fprintf(dest_file, "// ModuTitle: Table of toupper and tolower\n");
	fprintf(dest_file, "// LastCheck: RFZ03\n");
	fprintf(dest_file, "// OpLicense: %s\n", copyright_info);
	fprintf(dest_file, "\n");
	// tolower
	fprintf(dest_file, "const unsigned char _tab_tolower[256] = {\n");
	for (int i = 0; i < 16; i++) {
		for(int j = 0; j < 16; j++) {
			fprintf(dest_file, "\t%4d,", tolower(i * 16 + j));
		}
		fprintf(dest_file, "\n");
	}
	fprintf(dest_file, "};\n");
	// toupper
	fprintf(dest_file, "const unsigned char _tab_toupper[256] = {\n");
	for (int i = 0; i < 16; i++) {
		for(int j = 0; j < 16; j++) {
			fprintf(dest_file, "\t%4d,", toupper(i * 16 + j));
		}
		fprintf(dest_file, "\n");
	}
	fprintf(dest_file, "};\n");
	// 
	fprintf(dest_file, "const unsigned char _tab_alnum_digit[128] = {\n");
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 16; j++) {
			char ch = i * 16 + j;
			if (isdigit(ch))
				fprintf(dest_file, "\t%4d,", ch - '0');
			else if (isupper(ch))
				fprintf(dest_file, "\t%4d,", ch - 'A' + 10);
			else if (islower(ch))
				fprintf(dest_file, "\t%4d,", ch - 'a' + 10);
			else fprintf(dest_file, "\t%4d,", 0);
		}
		fprintf(dest_file, "\n");
	}
	fprintf(dest_file, "};\n");
	//
	fprintf(dest_file, "\n");
	fclose(dest_file);
	return 0;
}



