// ASCII C99 TAB4 CRLF
// Docutitle: (Algorithm) Fourier Dimension Transform : Time and Frequency 
// Codifiers: @dosconio: 20240708
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

#include "../../../inc/c/algorithm/frequency/Fourier.h"

//{TODO} detail

void dflFourier(dfloat* dat, stduint exp, double(*_loc_sin)(double))
{
	if (!_loc_sin) _loc_sin = dblsin;
	if (!exp) return; // size 1
	// dfloat 
	stduint siz = intpow2_iexpo(exp);
	stduint ida = siz / 2, idb = siz - 1;
	stduint tmp = 0;
	for0(i, idb) {
		if (i < tmp) {
			dfloat xtmp = dat[i];
			AssignParallel(dat[i], dat[tmp], xtmp);
		}
		stduint j = ida;
		while (j <= tmp) {
			tmp -= j;
			j >>= 1;
		}
		tmp += j;
	}
	stduint times = intlog2_iexpo(siz);
	for1(i, times) {
		stduint butterfly_distance = 1 << i;
		dfloat xtmp, ytmp;
		xtmp.real = 1.0; xtmp.imag = 0.0; // cofficient of butterfly
		ytmp.real = _loc_cos(_VAL_PI / (butterfly_distance / 2));
		ytmp.imag = -_loc_sin(_VAL_PI / (butterfly_distance / 2));
		for0(j, butterfly_distance / 2) {
			for (stduint k = j; k < siz; k += butterfly_distance) {
				const stduint ip = k + butterfly_distance / 2;
				dfloat ztmp = dflmul(dat[ip], xtmp);
				dat[ip] = dflsub(dat[k], ztmp);
				dat[k] = dfladd(dat[k], ztmp);
			}
			xtmp = dflmul(xtmp, ytmp);
		}
	}



}


