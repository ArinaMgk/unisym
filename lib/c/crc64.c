// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ05
// AllAuthor: @dosconio
// ModuTitle: Cyclic Redundancy Check with 64-bit result
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

#include "../../inc/c/crc64.h"
#include "../../inc/c/binary.h"
#include "limits.h"

uint64_t HashCRC64Bytes(const byte* data, size_t length, uint64_t crc, uint64_t polynomial, uint64_t final_xor, int refl)
{
	uint64_t remainder = crc;
	for (size_t byt = 0; byt < length; byt++)
	{
		remainder ^= BitReflect64(CHAR_BIT & refl, data[byt]) << 56;
		for (uint8_t bit = CHAR_BIT; bit > 0; bit--)
			remainder = (remainder << 1) ^ ((remainder & (1ULL << 63)) ? polynomial : 0);
	}
	return (BitReflect64(64 & refl, remainder) ^ final_xor);
}

uint64_t HashCRC64File(FILE* fptr, uint64_t crc, uint64_t polynomial, uint64_t final_xor, int refl)
{
	fpos_t fpos;
	int chr;
	fgetpos(fptr, &fpos);
	fseek(fptr, 0, SEEK_SET);
	while ((chr = fgetc(fptr)) != EOF)
		crc = HashCRC64Once(crc, chr, polynomial, refl);
	crc = HashCRC64Endo(crc, CRC64_ECMA_finalxor, refl);
	fsetpos(fptr, &fpos);
	return crc;
}

uint64_t HashCRC64Once(uint64_t last, byte data, uint64_t polynomial, int refl)
{
	last ^= BitReflect64(CHAR_BIT & refl, data) << 56;
	for (uint8_t bit = CHAR_BIT; bit > 0; bit--)
		last = (last << 1) ^ ((last & (1ULL << 63)) ? polynomial : 0);
	return last;
}

uint64_t HashCRC64Endo(uint64_t last, uint64_t final_xor, int refl)
{
	return (BitReflect64(64 & refl, last) ^ final_xor);
}
