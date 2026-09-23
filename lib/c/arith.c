// ASCII CPP TAB4 CRLF
// Docutitle:  
// Datecheck: 20240420 ~ .
// Developer: @dosconio @ UNISYM
// Attribute: [Allocate]
// Reference: None
// Dependens: None
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

#include "../../inc/c/arith.h"
#include "../../inc/c/auxiliary/trigtab.h"
#include <float.h>

stduint _EFDIGS = 6;

// CORE of Arithmetic
const static double _pi = _VAL_PI;
const static double _ln2 = _VAL_L2;
const static double _ln10 = 2.30258509299404568402;
const static double _e = _VAL_E;
// ln16
const static double _4ln2 = 4 * _VAL_L2;

// ---NO REGISTER---
#define _ABS_IMM(a) ((a)<0?-(a):(a))
int intabs(int j) { return _ABS_IMM(j); }
int abs(int j) { return _ABS_IMM(j); }
long int labs(long int j) { return _ABS_IMM(j); }
long long int llabs(long long int j) { return _ABS_IMM(j); }
// ---NO REGISTER--- END

void ariprecise(stduint prec)
{
	_EFDIGS = prec;
}

stduint intFibonacci(stduint idx)
{
	// 0, 1, 1, 2, 3, 5, ...
	if (idx < 2) return idx;
	idx -= 2;
	stduint a = 1, b = 1, c = 1;
	while (idx)
	{
		c = a + b;
		a = b;
		b = c;
		--idx;
	}
	return c;
}

double dblpow_iexpo(double bas, stdint exp)
{
	if (exp < 0) return 1 / dblpow_iexpo(bas, -exp);
	double ret = 1.0;
	while (exp)
	{
		(exp & 1) && (ret *= bas);
		exp >>= 1;
		bas *= bas;
	}
	return ret;
}

static double dblexp_recurs(double floatings, double res_parti)
{
	// exp(x)= 1 + x + x^2/2! + x^3/3!
	stduint i = 1;
	double divr = 1.;
	double res = res_parti;
	double plus = res_parti;
	double efdigs = dblpow_iexpo(10., _EFDIGS + 4);
	int conti = 1;
	do
	{
		plus *= floatings / i++;
		res += plus;
		conti = plus * efdigs >= res;
	} while (conti);
	return res;
}
double dblexp(double expo)
{
	if (expo < 0) return 1. / dblexp(-expo);
	stduint intepart = (stduint)expo;
	expo -= intepart;
	// cot$ exp(x+y)=exp(x)exp(y)
	double res_parti = dblpow_iexpo(_e, intepart);
	return dblexp_recurs(expo, res_parti);
}


//{TODO}{DETAIL} Simpson Method
	// Int_a^b{f(x)}=(b-a)*{f(a)+4*avg(a,b)+f(b)}/6
	// log(x)=Int_1^x{1/t}
static double simpson(double limdn, double limup, double(*fn)(double))
{
	return (fn(limdn) + 4 * fn(limdn / 2 + limup / 2) + fn(limup)) * (limup - limdn) / 6;
}
static double sumsimpson_recurs(double dn, double up, double flt_divs, double a, double(*fn)(double))
{
	// ?
	double c = dn / 2 + up / 2;
	double L = simpson(dn, c, fn);
	double R = simpson(c, up, fn);
	if (dblabs(L + R - a) <= 15 * flt_divs)
		return L + R + (L + R - a); //{TODO} 15 wo where?
	return sumsimpson_recurs(dn, c, flt_divs / 2, L, fn) + sumsimpson_recurs(c, up, flt_divs / 2, R, fn);
}
static double sumsimpson(double dn, double up, double(*fn)(double))
{
	return sumsimpson_recurs(dn, up, dblpow_iexpo(0.1, _EFDIGS + 4), simpson(dn, up, fn), fn);
}

// Simpson Method
static double dbllog_simpson_sub(const double inp)
{
	return 1 / inp;
}
/* // Taylor Method
static double dbllog_recurs(double floatings)// inp-range (-1,1]
{
	// log(1+x)= x - x^2/2 + x^3/3
	stduint i = 1;
	double divr = 1.;
	double res = 0.0;
	double plus = 1.0;
	double efdigs = dblpow_iexpo(10., _EFDIGS + 4);
	int conti = 1;
	if (floatings) do
	{
		plus *= floatings;
		res += plus / i;
		conti = (dblabs(plus) * efdigs) >= (i * dblabs(res));
		i++; plus *= -1;
	} while (conti);
	return res;
}*/
double dbllog(double power)
{
	return sumsimpson(1, power, dbllog_simpson_sub);
	/*
	if (power < 0)
	{
		double a = 0, b = 0;
		return a / b;
	}
	if (power == 1) return 0;
	if (power == 2) return _ln2;
	return (power < 0.01 ) ? dbllog_recurs(power - 1) :
		_4ln2 + dbllog(power / 16);
	*/
}

double dblpow_fexpo(double base, double expo)
{
	
	//_EFDIGS += 1;//{TEMP} This is my feeling
	double res = dblexp(dbllog(base) * expo);
	//_EFDIGS -= 1;
	return res;
}

double dblabs(double inp) { return inp < 0 ? -inp : inp; }

double dblfloor(double inp)
{
	int64 trunc = (int64)inp;
	double res = (double)trunc;
	if (res > inp) res -= 1.0;
	return res;
}

double dblsqrt(double inp)
{
	//{TODO} fast-invsqrt-explained ?
	return dblpow_fexpo(inp, .5);
}

double dblsin(double rad)
{
	double s, c;
	dblsincos(rad, &s, &c);
	return s;
}
double dblcos(double rad)
{
	double s, c;
	dblsincos(rad, &s, &c);
	return c;
}

// ---- trigonometric range reduction ----
// rad = n * (pi/2) + r with |r| <= pi/4, carrying pi/2 and 2/pi as double-double
// (106 bit). The reduction error then stays on the level of the final rounding
// instead of growing with |rad|: the former `rad -= k*pi` lost k*ulp(pi) and
// cost about 1e-8 absolute (5.8e-3 relative) already at |rad| = 1e9.
#define _VAL_PIO2_HI 0x1.921fb54442d18p+0
#define _VAL_PIO2_LO 0x1.1a62633145c07p-54
#define _VAL_2OPI_HI 0x1.45f306dc9c883p-1
#define _VAL_2OPI_LO -0x1.6b01ec5417056p-55
#define _VAL_TRIG_SPLIT 134217729.0 // 2^27 + 1, for the Dekker splitting

// Dekker: p = a*b and e = the exact residual of that rounding
static void dbltwoprod(double a, double b, double* p, double* e)
{
	double ca = _VAL_TRIG_SPLIT * a, ahi = ca - (ca - a), alo = a - ahi;
	double cb = _VAL_TRIG_SPLIT * b, bhi = cb - (cb - b), blo = b - bhi;
	double t = a * b;
	*p = t;
	*e = ((ahi * bhi - t) + ahi * blo + alo * bhi) + alo * blo;
}

void dblsincos(double rad, double* out_sin, double* out_cos)
{
	const double step_hi = _VAL_PIO2_HI / (double)TRIGTAB_QUARTER_N;
	const double step_lo = _VAL_PIO2_LO / (double)TRIGTAB_QUARTER_N;
	double q, qe, t, nf, fr, ph, pl, d, r, ar, d2, sd, cd, sinr, cosr;
	double tab_s, tab_c, sinv, cosv;
	stduint k;
	int64 ni;
	int quad;
	// n = nearest integer of rad * (2/pi)
	dbltwoprod(rad, _VAL_2OPI_HI, &q, &qe);
	qe += rad * _VAL_2OPI_LO;
	t = q + 0.5; // exact for |q| < 2^52
	nf = dblfloor(t);
	fr = t - nf; // exact fractional part, decides the tie together with qe
	if (fr + qe <= 0.0) nf -= 1.0;
	else if (fr + qe >= 1.0) nf += 1.0;
	ni = (int64)nf;
	quad = (int)(((ni % 4) + 4) % 4);
	// r = rad - n * (pi/2)
	dbltwoprod(nf, _VAL_PIO2_HI, &ph, &pl);
	pl += nf * _VAL_PIO2_LO;
	d = rad - ph; // exact by Sterbenz, the two operands cancel
	r = d - pl;
	// table + taylor for |r| <= pi/4
	ar = r < 0 ? -r : r;
	k = (stduint)(ar / step_hi + 0.5);
	if (k > TRIGTAB_QUARTER_N) k = TRIGTAB_QUARTER_N;
	dbltwoprod((double)k, step_hi, &ph, &pl);
	d = ((ar - ph) - pl) - (double)k * step_lo;
	d2 = d * d;
	sd = d + d * d2 * (-1.0 / 6.0 + d2 * (1.0 / 120.0));
	cd = 1.0 + d2 * (-1.0 / 2.0 + d2 * (1.0 / 24.0 - d2 * (1.0 / 720.0)));
	tab_s = (double)_tab_sin_quarter[k];
	tab_c = (double)_tab_sin_quarter[TRIGTAB_QUARTER_N - k];
	sinr = tab_s * cd + tab_c * sd;
	cosr = tab_c * cd - tab_s * sd;
	if (r < 0) sinr = -sinr;
	switch (quad)
	{
	case 0: sinv = sinr; cosv = cosr; break;
	case 1: sinv = cosr; cosv = -sinr; break;
	case 2: sinv = -sinr; cosv = -cosr; break;
	default: sinv = -cosr; cosv = sinr; break;
	}
	if (out_sin) *out_sin = sinv;
	if (out_cos) *out_cos = cosv;
}

double dbltan(double rad)
{
	double s, c;
	dblsincos(rad, &s, &c);
	return s / c;
}

// asin(x) = \Int_0^x{dt/sqrt(1-t^2)}
static double dblasin_simpson_sub(double val)
{
	return 1 / dblsqrt(1 - val * val);
}
double dblasin(double val)
{
	if (dblabs(val) > 1)
	{
		double a = 0, b = 0;
		return a / b;
	}
	double res = val < 0 ? -1 : 1;
	val = dblabs(val);
	if (val == 1) return res * _pi / 2;
	return res * sumsimpson(0, val, dblasin_simpson_sub);
}

double dblacos(double val)
{
	if (dblabs(val) > 1)
	{
		double a = 0, b = 0;
		return a / b;
	}
	return _pi / 2 - dblasin(val);
}

static double dblatan_recurs(double inp)
{
	// tan(x)= x - x^3/3 + x^5/5 ...
	stduint i = 1;
	double res = inp;
	double plus = inp;
	const double inpsqu = inp * inp;
	double efdigs = dblpow_iexpo(10., _EFDIGS + 2);
	int conti = 1;
	do
	{
		plus *= inpsqu * (-1);
		i += 2;
		res += plus / i;
		conti = dblabs(plus) * efdigs >= dblabs(res) * i;
	} while (conti);
	return res;
}
double dblatan(double val)
{
	if (val < 0) return -dblatan(-val);
	//{TODO} 2atan(x) = atan(2x/(1-x^2))
	if (val > 1) return _pi / 2 - dblatan(1 / val);
	return dblatan_recurs(val);
}

double dbllog10(double power)
{
	return dbllog(power) / _ln10;
}

double dblsinh(double rad)
{
	return (dblexp(rad) - dblexp(-rad)) / 2;
}

double dblcosh(double rad)
{
	return (dblexp(rad) + dblexp(-rad)) / 2;
}

double dbltanh(double rad)
{
	return 1 - 2 / (1 + dblpow_iexpo(dblexp(rad), 2));
}

double dblasinh(double val)
{
	return dbllog(val + dblsqrt(val * val + 1));
}

double dblacosh(double val)
{
	if (val < 1)
	{
		double a = 0, b = 0;
		return a / b;
	}
	if (val == 1) return 0;
	return dbllog(val + dblsqrt(val * val - 1));
}

double dblatanh(double val)
{
	if (dblabs(val) > 1)
	{
		double a = 0, b = 0;
		return a / b;
	}
	return dbllog((1 + val) / (1 - val)) / 2;
}

