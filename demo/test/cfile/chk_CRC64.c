// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ05
// AllAuthor: @dosconio
// ModuTitle: Test for Cyclic Redundancy Check with 64-bit result
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

#include "crc64.h"
#include <stdio.h>
#include <stdlib.h>

FILE* file;
uint64_t crc;
int main() {
	if ((file = fopen(__FILE__, "rb")) == NULL)
		return 1;
	
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	unsigned char* buffer = malloc(file_size);
	if (buffer == NULL)
		goto endo;
	if (fread(buffer, file_size, 1, file) != 1)
		goto endo;

	crc = CRC64_ECMA_Initial;
	for (unsigned i = 0; i < file_size; i++)
		crc = HashCRC64Once(crc, buffer[i], CRC64_ECMA_Polynomial, -1);
	crc = HashCRC64Endo(crc, CRC64_ECMA_finalxor, -1);
	printf("CRC64 of " __FILE__ ": %016llX\n", crc);

	crc = HashCRC64Bytes(buffer, file_size, CRC64_ECMA_Initial, CRC64_ECMA_Polynomial, CRC64_ECMA_finalxor, -1);
	printf("CRC64 of " __FILE__ ": %016llX\n", crc);
	

	crc = HashCRC64File(file, CRC64_ECMA_Initial, CRC64_ECMA_Polynomial, CRC64_ECMA_finalxor, -1);
	printf("CRC64 of " __FILE__ ": %016llX\n", crc);

endo:
	free(buffer);
	fclose(file);
	return 0;
}
