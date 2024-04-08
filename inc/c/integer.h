// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant since(C99)
// LastCheck: 2024MAR29
// AllAuthor: @dosconio
// ModuTitle: Alias for ISO IEC Standard CPL string.h
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

// Wrote according to the standard C99
// GCC, MSVC-2022

#ifndef _INC_INTEGER
#define _INC_INTEGER

#include <limits.h>
#include <stddef.h>

// ---- ---- ---- ---- Core of UniSym Integer ---- ---- ---- ----

#if SIZE_MAX==0xFFFF
	#define _BINARY 16 
	#define __BIT_STR__ "16"
#elif SIZE_MAX==0xFFFFFFFF
	#define _BINARY 32
	#define __BIT_STR__ "32"
#elif SIZE_MAX==0xFFFFFFFFFFFFFFFF
	#define _BINARY 64
	#define __BIT_STR__ "64"
#else 
	#define _BINARY 8
	#define __BIT_STR__ "8"
#endif

#if defined(_WinNT)&&(_BINARY==64)
	#include "us_win64.h"
#else
	#include <stddef.h>//{TEMP}
	#include <stdint.h>//{TEMP}
	#if !defined(uint)&&!defined(_Linux)&&!defined(_NO_uint)// avoid GCC duplicate 'unsigned'
		#define uint unsigned int
	#endif
	#define llong long long int 
	#define sll signed long long int
	#define ullong unsigned long long int
	#define ull unsigned long long int
	typedef unsigned char byte; // [MinGW-i686 Conflict] #define byte unsigned char
	typedef unsigned char uint8; //[trend] [MinGW-i686 Conflict] #define byte unsigned char
	typedef   signed char sbyte;
	typedef uint16_t  word;// unsigned short int
	typedef uint16_t  uint16;//[trend] unsigned short int
	typedef  int16_t  sword;// signed short int
	typedef uint32_t  dword;// unsigned int
	typedef uint32_t  uint32;//[trend] unsigned int
	typedef  int32_t  sdword;// signed int
	typedef uint64_t  qword;// unsigned long long int
	typedef uint64_t  uint64;//[trend] unsigned long long int
	typedef  int64_t  sqword;// signed long long int
	typedef size_t    stduint;
	typedef ptrdiff_t stdint ;
	//
	#define valword(x) (*(word*)&(x))// will be template overload as C++ version
#endif

// ---- ---- ---- ---- inttypes.h ---- ---- ---- ----

// The fprintf macros for integers are:

#define PRId8  "d"
#define PRId16 "d"
#define PRId32 "d"
#define PRId64 "lld"
#define PRIi8  "i"
#define PRIi16 "i"
#define PRIi32 "i"
#define PRIi64 "lli"

#define PRIo8  "o"
#define PRIo16 "o"
#define PRIo32 "o"
#define PRIo64 "llo"
#define PRIu8  "u"
#define PRIu16 "u"
#define PRIu32 "u"
#define PRIu64 "llu"
#define PRIx8  "x"
#define PRIx16 "x"
#define PRIx32 "x"
#define PRIx64 "llx"
#define PRIX8  "X"
#define PRIX16 "X"
#define PRIX32 "X"
#define PRIX64 "llX"

#define PRIdLEAST8  "d"
#define PRIdLEAST16 "d"
#define PRIdLEAST32 "d"
#define PRIdLEAST64 PRId64
#define PRIiLEAST8  "i"
#define PRIiLEAST16 "i"
#define PRIiLEAST32 "i"
#define PRIiLEAST64 PRIi64
#define PRIoLEAST8  "o"
#define PRIoLEAST16 "o"
#define PRIoLEAST32 "o"
#define PRIoLEAST64 PRIo64
#define PRIuLEAST8  "u"
#define PRIuLEAST16 "u"
#define PRIuLEAST32 "u"
#define PRIuLEAST64 PRIu64
#define PRIxLEAST8  "x"
#define PRIxLEAST16 "x"
#define PRIxLEAST32 "x"
#define PRIxLEAST64 PRIx64
#define PRIXLEAST8  "X"
#define PRIXLEAST16 "X"
#define PRIXLEAST32 "X"
#define PRIXLEAST64 PRIX64

#define PRIdFAST8  "d"
#define PRIdFAST16 "d"
#define PRIdFAST32 "d"
#define PRIdFAST64 PRId64
#define PRIiFAST8  "i"
#define PRIiFAST16 "i"
#define PRIiFAST32 "i"
#define PRIiFAST64 PRIi64
#define PRIoFAST8  "o"
#define PRIoFAST16 "o"
#define PRIoFAST32 "o"
#define PRIoFAST64 PRIo64
#define PRIuFAST8  "u"
#define PRIuFAST16 "u"
#define PRIuFAST32 "u"
#define PRIuFAST64 PRIu64
#define PRIxFAST8  "x"
#define PRIxFAST16 "x"
#define PRIxFAST32 "x"
#define PRIxFAST64 PRIx64
#define PRIXFAST8  "X"
#define PRIXFAST16 "X"
#define PRIXFAST32 "X"
#define PRIXFAST64 PRIX64

#define PRIdMAX PRId64
#define PRIiMAX PRIi64
#define PRIoMAX PRIo64
#define PRIuMAX PRIu64
#define PRIxMAX PRIx64
#define PRIXMAX PRIX64

#define PRIdPTR PRId64
#define PRIiPTR PRIi64
#define PRIoPTR PRIo64
#define PRIuPTR PRIu64
#define PRIxPTR PRIx64
#define PRIXPTR PRIX64

// The fscanf macros for integers are:

#define SCNd8 "hhd"
#define SCNd16 "hd"
#define SCNd32 "d"
#define SCNd64 PRId64
#define SCNi8 "hhi"
#define SCNi16 "hi"
#define SCNi32 "i"
#define SCNi64 PRIi64

#define SCNo8 "hho"
#define SCNo16 "ho"
#define SCNo32 "o"
#define SCNo64 PRIo64
#define SCNu8 "hhu"
#define SCNu16 "hu"
#define SCNu32 "u"
#define SCNu64 PRIu64
#define SCNx8 "hhx"
#define SCNx16 "hx"
#define SCNx32 "x"
#define SCNx64 PRIx64

#define SCNdLEAST8  "hhd"
#define SCNdLEAST16 "hd"
#define SCNdLEAST32 "d"
#define SCNdLEAST64 PRId64
#define SCNiLEAST8  "hhi"
#define SCNiLEAST16 "hi"
#define SCNiLEAST32 "i"
#define SCNiLEAST64 PRIi64
#define SCNoLEAST8  "hho"
#define SCNoLEAST16 "ho"
#define SCNoLEAST32 "o"
#define SCNoLEAST64 PRIo64
#define SCNuLEAST8  "hhu"
#define SCNuLEAST16 "hu"
#define SCNuLEAST32 "u"
#define SCNuLEAST64 PRIu64
#define SCNxLEAST8  "hhx"
#define SCNxLEAST16 "hx"
#define SCNxLEAST32 "x"
#define SCNxLEAST64 PRIx64

#define SCNdFAST8  "hhd"
#define SCNdFAST16 "hd"
#define SCNdFAST32 "d"
#define SCNdFAST64 PRId64
#define SCNiFAST8  "hhi"
#define SCNiFAST16 "hi"
#define SCNiFAST32 "i"
#define SCNiFAST64 PRIi64
#define SCNoFAST8  "hho"
#define SCNoFAST16 "ho"
#define SCNoFAST32 "o"
#define SCNoFAST64 PRIo64
#define SCNuFAST8  "hhu"
#define SCNuFAST16 "hu"
#define SCNuFAST32 "u"
#define SCNuFAST64 PRIu64
#define SCNxFAST8  "hhx"
#define SCNxFAST16 "hx"
#define SCNxFAST32 "x"
#define SCNxFAST64 PRIx64

#define SCNdMAX PRId64
#define SCNiMAX PRIi64
#define SCNoMAX PRIo64
#define SCNuMAX PRIu64
#define SCNxMAX PRIx64

#define SCNdPTR PRId64
#define SCNiPTR PRIi64
#define SCNoPTR PRIo64
#define SCNxPTR PRIx64
#define SCNuPTR PRIu64

// International standard functions:

//struct imaxdiv_t
//{
//	intmax_t quo;
//	intmax_t rem;
//};

//#define imaxabs MAX//{TEMP}
//#define IntDivide imaxdiv 
//#define strtoimax atoins
//#define strtoumax atouin
//#define wcstoimax//{TEMP}
//#define wcstoumax//{TEMP}

// UniSym Special:

#define PRIdSTD PRIdPTR
#define PRIuSTD PRIuPTR
#define PRIxSTD PRIxPTR

// Using `max` `min` may be conflict with stdlib.h:
#ifndef maxof//(a,b)
#define maxof(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef minof//(a,b)
#define minof(a,b) ((a)<(b)?(a):(b))
#endif

// Intel Assembly Style
#define MAX(d,s) if((d)<(s)){(d)=(s);}
#define MIN(d,s) if((d)>(s)){(d)=(s);}

#define isodd(x) ((x)&1)
#define iseven(x) !((x)&1)

// More:

// para:direction [0:ascend 1:descend]
#define IntBubbleSort(array, size, direction) \
	for (size_t i = 0; i < (size) - 1; i++) \
		for(size_t j = i + 1; j < (size); j++) \
			if ((array[i] > array[j]) ^ (direction)) \
				array[i] ^= array[j] ^= array[i] ^= array[j];
// E.g. IntBubbleSort(my_ints, numsof(my_ints), 0);

#endif
