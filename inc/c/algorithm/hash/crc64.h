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

#ifndef _INC_HASH_CRC64
#define _INC_HASH_CRC64

#define CRC64_ARINA_Polynomial    0x6E6552616E696850// (PhinaRen)
#define CRC64_ARINA_Initial       0xFFFFFFFFFFFFFFFF// (PhinaRen)

#define CRC64_DOSCONIO_Polynomial 0x4F494E4F43534F44// (DOSCONIO)
#define CRC64_DOSCONIO_Initial    0xFFFFFFFFFFFFFFFF// (DOSCONIO)

#define CRC64_ECMA_Polynomial     0x42F0E1EBA9EA3693// (ECMA) Apply: 7-zip
#define CRC64_ECMA_Initial        0xFFFFFFFFFFFFFFFF// (ECMA)
#define CRC64_ECMA_finalxor	      0xFFFFFFFFFFFFFFFF// (ECMA)

#define CRC64_ISO_Polynomial      0x000000000000001B// (ISO)
#define CRC64_ISO_Initial         0xFFFFFFFFFFFFFFFF// (ISO)
#define CRC64_ISO_finalxor        0xFFFFFFFFFFFFFFFF// (ISO)

#define CRC64_JONES_Polynomial    0xAD93D23594C935A9// (JONES)
#define CRC64_JONES_Initial       0xFFFFFFFFFFFFFFFF// (JONES)
#define CRC64_WE_Polynomial       0x42F0E1EBA9EA3693// (WE)
#define CRC64_WE_Initial          0xFFFFFFFFFFFFFFFF// (WE)
#define CRC64_WE_MAV_Polynomial   0x42F0E1EBA9EA3693// (WE-MAV)
#define CRC64_WE_MAV_Initial      0x0000000000000000// (WE-MAV)
#define CRC64_GoISO_Polynomial    0x000000000000001B// (GoISO)
#define CRC64_GoISO_Initial       0xFFFFFFFFFFFFFFFF// (GoISO)
#define CRC64_Jones_4_Polynomial  0xD800000000000000// (Jones-4)
#define CRC64_Jones_4_Initial     0xFFFFFFFFFFFFFFFF// (Jones-4)
#define CRC64_T10DIF_Polynomial   0x000000000000001B// (T10DIF)
#define CRC64_T10DIF_Initial      0xFFFFFFFFFFFFFFFF// (T10DIF)
#define CRC64_ECMA_182_Polynomial 0x42F0E1EBA9EA3693// (ECMA-182)
#define CRC64_ECMA_182_Initial    0x0000000000000000// (ECMA-182)
#define CRC64_USB_Polynomial      0x000000000000001B// (USB)
#define CRC64_USB_Initial         0xFFFFFFFFFFFFFFFF// (USB)

#include <stdio.h>
#include "../../stdinc.h"

//
uint64_t HashCRC64Bytes(const byte* data, size_t length, uint64_t crc, uint64_t polynomial, uint64_t final_xor, int refl);

//
uint64_t HashCRC64File(FILE* fptr, uint64_t crc, uint64_t polynomial, uint64_t final_xor, int refl);

//
uint64_t HashCRC64Once(uint64_t last, byte data, uint64_t polynomial, int refl);

//
uint64_t HashCRC64Endo(uint64_t last, uint64_t final_xor, int refl);

#endif // !_INC_HASH_CRC64
