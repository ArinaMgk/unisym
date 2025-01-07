// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant Host[Allocation]
// LastCheck: RFZ23
// AllAuthor: @ArinaMgk
// ModuTitle: from CoeTaylor
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
#include "../../../inc/c/ustdbool.h"
#if !defined(_MCCA)
#include <math.h>

double dbltaylor(double inp, enum TaylorType dptor_in, double period)
{
	byte dptor = dptor_in;
	if (inp == 0.0 && !(_CDETayDptr_PoStart(dptor)))// {TEMP} for exp() dpot(0)
		return 1.0;

	double crt = inp;
	if (period != 0.0)
	{
		while (crt < 0) crt += period;
		while (crt >= period) crt -= period;
	}
	double CrtPow = (_CDETayDptr_PoStart(dptor));
	//boolean sign = crt < 0;
	boolean signcrt = _CDETayDptr_PowSign(dptor);// ^ sign
	boolean signshock = _CDETayDptr_SignTog(dptor);
	enum { all_ = 0, only_odd, only_even, none_ } powtype = _CDETayDptr_PowType(dptor);
	if (powtype == none_) goto endo;
	enum { one_, n_, nf_, nn_ } divtype = _CDETayDptr_DivType(dptor);
	if (divtype == nn_) goto endo;// do not support NOW.
	double result = 0.0;
	double plus;
	double diver = divtype == one_ ? 1.0 :
		divtype == n_ ? CrtPow :
		divtype == nf_ ? dblfactorial(CrtPow) : 0;

	// += sgn* powx/divcof
	size_t loops = 0;
	lup_last = 0;
	do
	{
		lup_last++;
		plus = crt;
		plus = dblpow_fexpo(plus, CrtPow) / diver;
		if (plus)
		{
			double ori = result;
			if (signcrt) plus = -plus;
			result += plus;
			if (signshock) signcrt = !signcrt;
			CrtPow += powtype == all_ ? 1 : 2;
			diver = divtype == one_ ? 1 :
				divtype == n_ ? CrtPow :
				divtype == nf_ ? dblfactorial(CrtPow) : 0;
		}
	} while (absof(result) <= absof(plus) * dblpow_fexpo(10., _EFDIGS));

	endo: return result;
}
#endif
