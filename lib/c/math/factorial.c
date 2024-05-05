// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: 20240410
// AllAuthor: @ArinaMgk
// ModuTitle: 
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

#include "../../../inc/c/arith.h"
#include <math.h>
// RFW9 Factorial family
size_t Factorial(size_t argu)// from SGAr
{
	if (!argu) return 1;
	size_t res = 1;
	while (argu > 1) res *= argu--;
	return res;
}
// RFW9 Factorial family
size_t Arrange(size_t total, size_t items)// Amn
{
	size_t res = 1;
	while (items)
	{
		res *= total--;
		items--;
	}
	return res;
}
// RFW9 Factorial family
size_t Combinate(size_t total, size_t items)// Cmn
{
	return Arrange(total, items) / Factorial(items);
}
// RFW9 Factorial family
int CombinateCheck(size_t bits, size_t items, size_t crt)
{
	//001001=crt=9 bits=6 items=2(num of 1)
	//abcdef
	//example, c and f will be the 2 chosen.
	//return 0 for failure.
	//use BTS may be better
	size_t set = 0;// num of 1
	size_t crt_mask = 1;
	while (bits--)// a waste effect : 0 -> -1
	{
		if (crt & crt_mask)
		{
			set++;
			if (set > items) return 0;
		}
		crt_mask <<= 1;// .shl 1
	}
	return (set == items);
}

double dblfactorial(double inp)
{
	double res = 1.0;
	inp = floor(inp);
	if (inp <= 1) return 1;
	while (inp >= 1)
	{
		res *= inp;
		inp--;	
	}
	return res;
}
