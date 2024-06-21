// ASCII CPL/CPP TAB4 CRLF
// Docutitle: Number (Complex)
// Codifiers: @dosconio: 20240603
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

#ifndef _INC_NUMBER
#define _INC_NUMBER

#include "floating.h"
#include "ustdbool.h"

// ---- Complex ---- 

struct ddouble /* double-double, ddi */ {
	double real, imag;
};

struct dfloat /* double-float, dfl */ {
	float real, imag;
};

// ---- CoeAr ---- 

// ..


// ---- HerAr ---- 
extern boolean sign_coff;
extern boolean sign_expo;
extern stduint byte_total;
extern stduint byte_divr;
extern stduint byte_expo;

#endif
