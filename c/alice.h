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
#ifndef ArnHabit
#define ArnHabit

#define nil 0
#define nop 0x90

#define masm __asm

#define print(x) printf("%s",(x));
#define printline(x) printf("%s\n",(x));
#define pll(x,y)   printf("%s : %llx\n",(x),(y));
#define pd(x) printf("%d\n",(x));

#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))

#define point(r,c) (r*cols+c)

#define uint unsigned int
#define llong long long int
#define sll signed long long int
#define ullong unsigned long long int
#define ull unsigned long long int
#define byte unsigned char
#define word unsigned short int
#define dword unsigned int
#define stduint size_t
#define stdint ptrdiff_t

#define jump goto
#define call(x) (x)()

// These IF can avoid error such as mixing "a=0" and "a==0"
#define If(con) if(0||con)
#define If_(con) if(1&&con)
#define RNZ(a) {if(a) return (a);}// Return if not zero
#define RZ(a) {if(!a) return (a);}// Return if zero

#define idle() {jump: goto jump;}

#define zalloc(x) calloc((x),1)// Zero Alloc

// Added RFW24, Exchange
#define xchg(a,b) (a)^=(b)^=(a)^=(b)
#define xchgptr(a,b) *(size_t*)&(a)^=*(size_t*)&(b)^=*(size_t*)&(a)^=*(size_t*)&(b)

#ifndef max//(a,b)
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min//(a,b)
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#endif
