// ASCII CPL/CPP TAB4 CRLF
// Docutitle: (Algorithm) Fourier Dimension Transform : Time and Frequency 
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

#ifndef _INCPP_Algorithm_Fourier
#define _INCPP_Algorithm_Fourier

#include "../../arith.h"

#define _loc_cos(x) _loc_sin((x)+_VAL_PI/2)

// dat = [dblsin(2 * PI * i / (F_s) * 10)]
//  where a one <=> F_s / NPT
void dflFourier(dfloat* dat, stduint num, double(*_loc_sin)(double));

#endif
