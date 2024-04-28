// ASCII C99 TAB4 CRLF
// LastCheck: 2023 Nov 16
// AllAuthor: @ArinaMgk; @dosconio
// ModuTitle: Processor inside Math / Arithmetic
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

#ifndef _INC_ARITHMETIC
#define _INC_ARITHMETIC

#include "stdinc.h"
#include "coear.h"

extern stduint _EFDIGS;// how many effective digits

double dblfactorial(double inp);

#define _CDETayFunc_Exp 0b10000000
#define _CDETayFunc_Sin 0b10010101
#define _CDETayFunc_Cos 0b10100001// cos yo (0,2]
#define _CDETayFunc_Log 0b01000101// ln(x+1)
#define _CDETayFunc_Atn 0b01010101// arctan [-1,+1]
#define _CDETayDptr_DivType(x) ((0b11000000&(x))>>6)
#define _CDETayDptr_PowType(x) ((0b00110000&(x))>>4)
#define _CDETayDptr_PoStart(x) ((0b00001100&(x))>>2)
#define _CDETayDptr_PowSign(x) ((0b00000010&(x))>>1)// [bit]
#define _CDETayDptr_SignTog(x) (0b00000001&(x))// [bit]
enum TaylorType {
	TaylorExp = _CDETayFunc_Exp,
	TaylorSin = _CDETayFunc_Sin,
	TaylorCos = _CDETayFunc_Cos,
	TaylorLog = _CDETayFunc_Log,
	TaylorAtn = _CDETayFunc_Atn,
};
// Taylors is writen by Haruno RFR.
//MSB[Divr 00:1 01:n 10:n! 11:n*n]
//   [00:All 01:OnlyOdd 10: OnlyEven]
//   [StartPower 0~3]
//   [StartSign 0:+ 1:-]
//LSB[SignFlap 0:N 1:Y]
double dbltaylor(double inp, enum TaylorType dptor, double period);

#endif
