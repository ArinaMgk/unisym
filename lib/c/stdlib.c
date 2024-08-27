// ASCII C TAB4 CRLF
// Docutitle: (Interface) STDLIB
// Codifiers: @dosconio: 20240602
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

#include "../../inc/c/ISO_IEC_STD/stdlib.h"

unchecked //{TODO} implementation by ASMx86(Hard) and (Soft) <- Decide by ARCHITECTURE

struct div_t div(int numer, int denom)
{
	struct div_t result;
	result.quot = numer / denom;
	result.rem = numer % denom;
	return result;
}

struct ldiv_t ldiv(long int numer, long int denom)
{
	struct ldiv_t result;
	result.quot = numer / denom;
	result.rem = numer % denom;
	return result;
}

struct lldiv_t lldiv(long long int numer, long long int denom)
{
	struct lldiv_t result;
	result.quot = numer / denom;
	result.rem = numer % denom;
	return result;
}
