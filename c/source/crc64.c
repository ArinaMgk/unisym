// ASCII C RFC07 @dosconio _LIB_ENCODE_CRC64
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
// Cyclic Redundancy Check with 64-bit result

#include "../crc64.h"
#include "limits.h"

//{TODO} Static CRC64 table system

uint64_t CRC64Reflect(uint64_t data, int width)
{
	uint64_t reflection = 0x0;
	if (!width) return data;
	for (int bit = 0; bit < width; bit++)
	{
		if (data & 0x01)
			reflection |= (1ULL << (width - 1 - bit));
		data >>= 1;
	}
	return reflection;
}

uint64_t CRC64(const unsigned char* data, size_t length, uint64_t crc, uint64_t polynomial, uint64_t final_xor, int refl)
{
	uint64_t remainder = crc;
	for (size_t byte = 0; byte < length; byte++)
	{
		remainder ^= (CRC64Reflect(data[byte], 8 & refl) << 56);
		for (uint8_t bit = CHAR_BIT; bit > 0; bit--)
			if (remainder & (1ULL << 63))
				remainder = (remainder << 1) ^ polynomial;
			else
				remainder <<= 1;
	}
	return (CRC64Reflect(remainder, 64 & refl) ^ final_xor);
}

uint64_t CRC64Once(uint64_t last, unsigned char data, uint64_t polynomial, int refl)
{
	last ^= CRC64Reflect(data, 8 & refl) << 56;
	for (uint8_t bit = CHAR_BIT; bit > 0; bit--)
		if (last & (1ULL << 63))
			last = (last << 1) ^ polynomial;
		else
			last <<= 1;
	return last;
}

uint64_t CRC64Endo(uint64_t last, uint64_t final_xor, int refl)
{
	return (CRC64Reflect(last, 64 & refl) ^ final_xor);
}
