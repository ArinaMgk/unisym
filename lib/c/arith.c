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
const static double _ln10 = 2.30258509299404568402;

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
		if (exp & 1) ret *= bas;
		exp >>= 1;
		bas *= bas;
	}
	return ret;
}

// ---- elementary function core ----
// Every reduction below carries its constants as a double-double and uses the
// Dekker/Knuth exact product and exact sum, so the reduction error stays at the
// level of the final rounding instead of growing with the argument. No lookup
// table is used: the range reduction comes from the exponent field (frexp /
// ldexp), which is where sin/cos need a table and log/exp do not.
#define _VAL_LN2_HI 0x1.62e42fefa39efp-1
#define _VAL_LN2_LO 0x1.abc9e3b39803fp-56
#define _VAL_INV_LN2 0x1.71547652b82fep+0
#define _VAL_INV_LN2_LO 0x1.777d0ffda0d24p-56
#define _VAL_PI_HI 0x1.921fb54442d18p+1
#define _VAL_PI_LO 0x1.1a62633145c07p-53
#define _VAL_PI2_HI 0x1.921fb54442d18p+0
#define _VAL_PI2_LO 0x1.1a62633145c07p-54
#define _VAL_PI4_HI 0x1.921fb54442d18p-1
#define _VAL_PI4_LO 0x1.1a62633145c07p-55
#define _VAL_SQRT2_2 0x1.6a09e667f3bcdp-1

static void dbltwoprod(double a, double b, double* p, double* e);

// Knuth two-sum: s = a+b with e the exact residual of that rounding
static void dbltwosum(double a, double b, double* s, double* e)
{
	double t = a + b, bb = t - a;
	*s = t;
	*e = (a - (t - bb)) + (b - bb);
}

static double dblinfp(void) // +inf, without dividing by zero
{
	union { double d; unsigned long long u; } v;
	v.u = 0x7FF0000000000000ULL;
	return v.d;
}
static double dblinfn(void) // -inf
{
	union { double d; unsigned long long u; } v;
	v.u = 0xFFF0000000000000ULL;
	return v.d;
}
static double dblnanp(void) // quiet NaN
{
	union { double d; unsigned long long u; } v;
	v.u = 0x7FF8000000000000ULL;
	return v.d;
}

static double dblpoly(const double* coef, int deg, double x)
{
	double res = coef[deg];
	while (deg--) res = res * x + coef[deg];
	return res;
}

// frexp/ldexp in pure bit arithmetic. The freestanding targets (the MCCA
// sysroot, Cortex-M) have no libm, and these two were the only host math calls
// the elementary functions still made. Denormals are handled by scaling up by
// 2^54 first, which is exact because it is a power of two.
double dblfrexp(double val, int* expo)
{
	union { double d; unsigned long long u; } v;
	int e;
	v.d = val;
	e = (int)((v.u >> 52) & 0x7FF);
	if (e == 0x7FF || val == 0.) // inf, nan, zero
	{
		*expo = 0;
		return val;
	}
	if (e == 0) // subnormal
	{
		double m = dblfrexp(val * 0x1p54, expo);
		*expo -= 54;
		return m;
	}
	v.u = (v.u & 0x800FFFFFFFFFFFFFULL) | 0x3FE0000000000000ULL; // == 0.5..1
	*expo = e - 1022;
	return v.d;
}

double dblldexp(double val, int expo)
{
	union { double d; unsigned long long u; } v;
	while (expo > 1023) { val *= 0x1p1023; expo -= 1023; }
	while (expo < -1022) { val *= 0x1p-1022; expo += 1022; }
	v.u = ((unsigned long long)(expo + 1023)) << 52; // 2^expo, expo now normal
	return val * v.d;
}

// Compensated Horner: p = poly(x) plus *err, the exact residual of the
// evaluation. A plain Horner on exp's 13 coefficients loses about 0.5 ulp of
// the polynomial value, which is most of what dblexp's error used to be.
static double dblpolyc(const double* coef, int deg, double x, double* err)
{
	double res = coef[deg], errs = 0.;
	int i;
	for (i = deg - 1; i >= 0; i--)
	{
		double pr, pe, sm, se;
		dbltwoprod(res, x, &pr, &pe);
		dbltwosum(pr, coef[i], &sm, &se);
		errs = errs * x + (pe + se);
		res = sm;
	}
	*err = errs;
	return res;
}

/* exp(r), |r| <= ln2/2, degree 12, max rel err 4.5e-19 */
static const double _poly_exp[13] = {
	0x1.0000000000000p+0, 0x1.0000000000000p+0, 0x1.0000000000000p-1,
	0x1.5555555555562p-3, 0x1.5555555555559p-5, 0x1.111111110db76p-7,
	0x1.6c16c16c14d77p-10, 0x1.a01a01b80226cp-13, 0x1.a01a01adc27e2p-16,
	0x1.71dde76ab2f81p-19, 0x1.27e4cc1847a65p-22, 0x1.af785d433f066p-26,
	0x1.1f8b43f2637adp-29,
};

/* Psi(z) for the plain log core, z = s^2 <= 0.1112, max abs err 2.2e-18 */
static const double _poly_logphi[12] = {
	-0x1.32b01e1beefb0p-64, 0x1.5555555555557p-1, 0x1.999999999948ap-2,
	0x1.24924924c738cp-2, 0x1.c71c71a36efecp-3, 0x1.745d1e598e9b3p-3,
	0x1.3b12ce1b3b2dfp-3, 0x1.1123ba81d26e6p-3, 0x1.dfd9a50e2f2f5p-4,
	0x1.c1a7b142e3b44p-4, 0x1.1db4d47413de0p-4, 0x1.51dc51ec51050p-3,
};

/* Psi(z) for log1p, in (z - 0.125) with z = s^2 <= 0.25, max abs err 8.4e-18.
   Centred because converting a Chebyshev series to monomials on an interval
   that does not straddle 0 is ill conditioned (see _poly_asin). */
static const double _poly_logphi_w[15] = {
	0x1.7177793d604dcp-4, 0x1.9068d5aa746b8p-1, 0x1.107b5198c8185p-1,
	0x1.bb01f6471d057p-2, 0x1.88be728aeb461p-2, 0x1.6e92a8aa17678p-2,
	0x1.6204636119bc4p-2, 0x1.5e4a60edb07cap-2, 0x1.60f3e32412821p-2,
	0x1.68b0be2655a41p-2, 0x1.74c2e3597337ep-2, 0x1.8412ae246e712p-2,
	0x1.97dc454b7381dp-2, 0x1.ce464d59f6a12p-2, 0x1.ec097ab6b6b1ap-2,
};

/* atan(u)/u - 1 in (u^2 - 0.28125), u^2 <= 0.5625, max abs err 7.5e-18 */
static const double _poly_atan[18] = {
	-0x1.49e6672dfc3b7p-4, -0x1.f9f99d58dbcdfp-3, 0x1.e0a5354830096p-4,
	-0x1.0e1c76081972fp-4, 0x1.49a3e695735bfp-5, -0x1.a687de4de63d4p-6,
	0x1.17caffb1b9aa7p-6, -0x1.7b4c527d4a857p-7, 0x1.05a34a2153c02p-7,
	-0x1.6de82e8933775p-8, 0x1.02ab0b3844e90p-8, -0x1.7101da457152ap-9,
	0x1.0962ae79e6fb3p-9, -0x1.7fd889eaebb97p-10, 0x1.11dcce25adbd9p-10,
	-0x1.90042e2fbcfd7p-11, 0x1.6bf4ff471271ap-11, -0x1.0c4456e12cb7dp-11,
};

/* (asin(x)-x)/x^3 in (x^2 - 0.28125), x^2 <= 0.5625, max abs err 6.2e-18.
   The wider first branch keeps the pi/2 - 2*asin(...) identity away from the
   point where its two terms have equal size, which is what cost 1.8 ulp. */
static const double _poly_asin[22] = {
	0x1.8984787951c62p-3, 0x1.c2cbb7c39ece1p-4, 0x1.631496886b59ap-4,
	0x1.45b22807d5bbep-4, 0x1.4678cdfe03b5cp-4, 0x1.5ac334e817769p-4,
	0x1.7f9ba172f56a1p-4, 0x1.b556fb3154725p-4, 0x1.fe4835d017a06p-4,
	0x1.2f3641231fdebp-3, 0x1.6dbe437a1c583p-3, 0x1.bea8e8c336c76p-3,
	0x1.13c170eb62db1p-2, 0x1.57513f69fbed8p-2, 0x1.aab5fdd1974d8p-2,
	0x1.0d5c2d2de2ef2p-1, 0x1.7468cdf692371p-1, 0x1.dc6e5113919f4p-1,
	0x1.467b05fcd3283p-1, 0x1.9f88bfee4627fp-1, 0x1.008d806406ea3p+2,
	0x1.4e65720eb94f9p+2,
};

/* (sinh(|x|)-|x|)/|x|^3 in |x|^2 - 1, |x|^2 <= 1, max abs err 1.2e-17 */
static const double _poly_sinh[7] = {
	0x1.5555555555555p-3, 0x1.11111111110fdp-7, 0x1.a01a01a01ee70p-13,
	0x1.71de3a4e14f40p-19, 0x1.ae6460fae3728p-26, 0x1.611cb330c1fe8p-33,
	0x1.b41232cf9950fp-41,
};

/* (cosh(|x|)-1)/|x|^2 in |x|^2 - 1, |x|^2 <= 1, max abs err 1.2e-17 */
static const double _poly_cosh[7] = {
	0x1.0000000000000p-1, 0x1.5555555555502p-5, 0x1.6c16c16c212c0p-10,
	0x1.a01a01907b7eep-16, 0x1.27e5069de9344p-22, 0x1.1ee5640e638a2p-29,
	0x1.99848a67700e4p-37,
};

/* (tanh(|x|)-|x|)/|x|^3 in |x|^2 - 1, |x|^2 <= 1, max abs err 1.9e-17 */
static const double _poly_tanh[16] = {
	-0x1.5555555555555p-2, 0x1.11111111110b9p-3, -0x1.ba1ba1ba1453ep-5,
	0x1.664f4880d2a47p-6, -0x1.226e3519865dfp-7, 0x1.d6d3c538d0bbep-9,
	-0x1.7da2bdc3daa27p-10, 0x1.3551ae34bdfffp-11, -0x1.f5211730ee038p-13,
	0x1.94981e5bdbe6fp-14, -0x1.42090e21d3d28p-15, 0x1.ea19b73b128d8p-17,
	-0x1.4eb911dcd9788p-18, 0x1.7238a0a7e22b6p-20, -0x1.1ab979c0ab58cp-22,
	0x1.b028f6cb72ebfp-26,
};

/* (expm1(x)-x)/x^2 in (x - 0.75), -0.5 <= x <= 2, max abs err 5.6e-17.
   Widened from [0,2] so that dblexpm1, dblsinh, dblcosh and dbltanh can stay
   on their polynomials further out instead of falling back on exp(x)-1. */
static const double _poly_expm1[16] = {
	0x1.4e0d33bc62353p-1, 0x1.f7a7fc6d7147ep-3, 0x1.0ea3c94f0d445p-4,
	0x1.c6297b06fe07dp-7, 0x1.393d54ce43204p-9, 0x1.6f3b9735857c3p-12,
	0x1.769942786edb1p-15, 0x1.5251aa50d59fdp-18, 0x1.1235c8b426ee1p-21,
	0x1.933872b961eedp-25, 0x1.0f4f0014154e7p-28, 0x1.5093bbcecb0d9p-32,
	0x1.833af2970e843p-36, 0x1.9f90bccddc95cp-40, 0x1.aa8ec7315c11cp-44,
	0x1.927f2286a21bep-48,
};

// exp of a double-double argument: exp(xh + xl)
static double dblexp_dd(double xh, double xl)
{
	double k, kh, kl, d, r, rlo, res;
	int i_expo;
	if (xh > 709.78271289338397) return dblinfp();
	if (xh < -745.13321910194111) return 0;
	k = dblfloor(xh * _VAL_INV_LN2 + 0.5); // rad = k*ln2 + r, |r| <= ln2/2
	i_expo = (int)k;
	dbltwoprod(k, _VAL_LN2_HI, &kh, &kl);
	kl += k * _VAL_LN2_LO;
	d = xh - kh; // exact: |d| <= |xh|/2
	r = d - kl;
	rlo = ((d - r) - kl) + xl;
	{
		double pe;
		res = dblpolyc(_poly_exp, 12, r, &pe);
		res += pe;
	}
	res += res * rlo;
	return dblldexp(res, i_expo); // 2^k goes into the exponent field
}

double dblexp(double expo)
{
	return dblexp_dd(expo, 0.);
}

double dblexp2(double val) // 2^val
{
	double h, l;
	dbltwoprod(val, _VAL_LN2_HI, &h, &l);
	l += val * _VAL_LN2_LO;
	return dblexp_dd(h, l);
}

double dblexpm1(double val)
{
	double t;
	if (val < -0.5)
	{
		// expm1(-y) = -expm1(y)/(1+expm1(y)): the direct x + x^2*R(x) form
		// would cancel badly for negative x once |x| is more than about 0.5
		if (val < -709.78271289338397) return -1.;
		t = dblexpm1(-val);
		return -t / (1. + t);
	}
	if (val > 709.78271289338397) return dblinfp();
	if (val > 2.) return dblexp(val) - 1.;
	return val + (val * val) * dblpoly(_poly_expm1, 15, val - 0.75);
}


//{TODO}{DETAIL} Simpson Method
	// Int_a^b{f(x)}=(b-a)*{f(a)+4*avg(a,b)+f(b)}/6
	// log(x)=Int_1^x{1/t}
// log(x) as a double-double: frexp gives x = m*2^k with m in [sqrt2/2, sqrt2),
// then an atanh-style series. Every cancellation is done with an exact
// sum/product so that the low word really carries information (pow needs that:
// |y|*log(x) must not lose |y| bits).
static void dbllog_dd(double power, double* out_hi, double* out_lo)
{
	int k;
	double m, f, s, R, hf, hfl, hfsq, A, Ae, B, Be, C, Ce, lm, lme, kh, kl, v, ve;
	m = dblfrexp(power, &k);
	if (m < _VAL_SQRT2_2) { m *= 2.; k -= 1; }
	f = m - 1.; // exact
	s = f / (2. + f); // |s| <= 0.17157
	R = dblpoly(_poly_logphi, 11, s * s);
	// log(1+f) = f - (hfsq - s*(hfsq+R)) with hfsq = f*f/2; the leading term f
	// is exact, so all the rounding of the division is scaled by hfsq
	dbltwoprod(f, f, &hf, &hfl);
	hfsq = 0.5 * hf;
	hfl = 0.5 * hfl;
	dbltwosum(hfsq, R, &A, &Ae); // A + Ae = hfsq + R
	Ae += hfl;
	dbltwoprod(s, A, &B, &Be); // B + Be = s*A
	Be += s * Ae;
	dbltwosum(hfsq, -B, &C, &Ce); // C + Ce = hfsq - B
	Ce += hfl - Be;
	dbltwosum(f, -C, &lm, &lme); // lm + lme = f - C
	lme += -Ce;
	dbltwoprod((double)k, _VAL_LN2_HI, &kh, &kl);
	kl += k * _VAL_LN2_LO;
	dbltwosum(kh, lm, &v, &ve);
	*out_hi = v;
	*out_lo = ve + lme + kl;
}

double dbllog(double power)
{
	double hi, lo;
	if (power < 0.) return dblnanp();
	if (power == 0.) return dblinfn();
	dbllog_dd(power, &hi, &lo);
	return hi + lo;
}

double dbllog1p(double val)
{
	double hi, lo, f, s, R, hf, hfl, hfsq, A, Ae, B, Be, C, Ce, lm, lme;
	if (val <= -1.) return (val == -1.) ? dblinfn() : dblnanp();
	if (val < -0.5 || val > 2.)
	{
		dbllog_dd(1. + val, &hi, &lo);
		return hi + lo;
	}
	// same arrangement as dbllog_dd, but here f = val is the exact leading
	// term, which is what keeps tiny |val| relatively accurate. The branch
	// reaches 2 so that dblasinh/dblacosh do not pay for rounding 1+t.
	f = val;
	s = f / (2. + f); // |s| <= 1/2 on this branch
	R = dblpoly(_poly_logphi_w, 14, s * s - 0.125);
	dbltwoprod(f, f, &hf, &hfl);
	hfsq = 0.5 * hf;
	hfl = 0.5 * hfl;
	dbltwosum(hfsq, R, &A, &Ae);
	Ae += hfl;
	dbltwoprod(s, A, &B, &Be);
	Be += s * Ae;
	dbltwosum(hfsq, -B, &C, &Ce);
	Ce += hfl - Be;
	dbltwosum(f, -C, &lm, &lme);
	return lm + (lme - Ce);
}

// log2(x) = log(x) / ln2, with both the log and the reciprocal of ln2 carried
// as double-doubles; a plain (hi+lo)*invln2 would throw the low word away.
double dbllog2(double val)
{
	double hi, lo, zh, zl;
	if (val < 0.) return dblnanp();
	if (val == 0.) return dblinfn();
	if (val > 1.7976931348623157e308) return dblinfp();
	if (val == 1.) return 0.;
	dbllog_dd(val, &hi, &lo);
	dbltwoprod(hi, _VAL_INV_LN2, &zh, &zl);
	zl += hi * _VAL_INV_LN2_LO + lo * _VAL_INV_LN2;
	return zh + zl;
}

// logb(x) = floor(log2|x|): dblfrexp gives |x| = m * 2^e with m in [0.5,1), so
// log2|x| = e + log2(m) lies in [e-1, e) and the floor is simply e-1. That
// also makes denormals come out right without a special case.
double dbllogb(double val)
{
	int e;
	if (val != val) return val;
	if (val == 0.) return dblinfn(); // logb(0) = -inf
	if (val > 1.7976931348623157e308 || val < -1.7976931348623157e308) return dblinfp();
	dblfrexp(val < 0. ? -val : val, &e);
	return (double)(e - 1);
}

double dblpow_fexpo(double base, double expo)
{
	double hi, lo, zh, zl, res;
	int iexpo;
	if (expo != expo) return expo;
	if (expo == 0.) return 1.;
	if (base != base) return base;
	if (base == 0.) return (expo > 0.) ? 0. : dblinfp();
	if (base < 0.)
	{
		// a real result needs an integral exponent; its parity gives the sign
		if (expo != dblfloor(expo)) return dblnanp();
		res = dblpow_fexpo(-base, expo);
		if (dblfloor(expo / 2.) * 2. != expo) res = -res;
		return res;
	}
	// short integral exponents: repeated squaring beats the log/exp route
	if (expo == dblfloor(expo) && expo >= -4. && expo <= 4.)
	{
		iexpo = (int)expo;
		if (iexpo < 0) return 1. / dblpow_fexpo(base, (double)(-iexpo));
		res = 1.;
		while (iexpo)
		{
			if (iexpo & 1) res *= base;
			iexpo >>= 1;
			if (iexpo) base *= base;
		}
		return res;
	}
	// exp(expo*log(base)) with the product carried as a double-double: without
	// that the |expo| bits of log(base) are simply lost
	dbllog_dd(base, &hi, &lo);
	dbltwoprod(expo, hi, &zh, &zl);
	zl += expo * lo;
	return dblexp_dd(zh, zl);
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
	union { double d; unsigned long long u; } v;
	double m, y, res;
	int i_expo;
	if (inp < 0.) return dblnanp();
	if (inp == 0. || inp > 1.7976931348623157e308) return inp;
	m = dblfrexp(inp, &i_expo); // inp = m * 2^expo, m in [0.5,1)
	m *= 2.; i_expo -= 1; // m in [1,2)
	if (i_expo & 1) { m *= 2.; i_expo -= 1; } // m in [2,4) or [1,2), even expo
	v.d = m;
	v.u = 0x5fe6eb50c7b537a9ULL - (v.u >> 1); // ~5% seed for 1/sqrt(m)
	y = v.d;
	y = y * (1.5 - 0.5 * m * y * y); // Newton: the error is squared each round
	y = y * (1.5 - 0.5 * m * y * y);
	y = y * (1.5 - 0.5 * m * y * y);
	y = y * (1.5 - 0.5 * m * y * y);
	res = m * y;
	res = 0.5 * (res + m / res); // polish down to the last bit
	return dblldexp(res, i_expo / 2);
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

// No numerical integration any more: a direct polynomial on |x| <= 1/2, and the
// angle-halving identity outside. The integrand 1/sqrt(1-t^2) is singular at
// t = +-1, which is exactly where the old Simpson route fell apart.
double dblasin(double val)
{
	double ax, az, res, a, v, e;
	int neg;
	neg = (val < 0.);
	ax = neg ? -val : val;
	if (ax > 1.) return dblnanp();
	if (ax == 1.) res = _VAL_PI2_HI + _VAL_PI2_LO;
	else if (ax <= 0.75)
	{
		az = ax * ax;
		res = ax + (ax * az) * dblpoly(_poly_asin, 21, az - 0.28125);
	}
	else // asin(x) = pi/2 - 2*asin(sqrt((1-x)/2))
	{
		a = dblasin(dblsqrt((1. - ax) * 0.5));
		dbltwosum(_VAL_PI2_HI, -2. * a, &v, &e);
		res = v + (e + _VAL_PI2_LO);
	}
	return neg ? -res : res;
}

double dblacos(double val)
{
	double ax, res, a, v, e;
	ax = (val < 0.) ? -val : val;
	if (ax > 1.) return dblnanp();
	if (ax <= 0.75) // no pi/2 - asin(x) cancellation here, and asin is small
	{
		dbltwosum(_VAL_PI2_HI, -dblasin(val), &v, &e);
		res = v + (e + _VAL_PI2_LO);
	}
	else if (val > 0.)
	{
		if (val == 1.) return 0.;
		res = 2. * dblasin(dblsqrt((1. - val) * 0.5));
	}
	else
	{
		if (val == -1.) return _VAL_PI_HI + _VAL_PI_LO;
		a = 2. * dblasin(dblsqrt((1. + val) * 0.5));
		dbltwosum(_VAL_PI_HI, -a, &v, &e);
		res = v + (e + _VAL_PI_LO);
	}
	return res;
}

// atan(x) = pi/4 + atan((x-1)/(x+1)) for |x| > 3/4: |u| <= 1/7 there, and at
// that point x-1 and x+1 are both exact (Sterbenz), so the branch adds almost
// no rounding of its own. The old |x| > 1/2 threshold made 1-x inexact for
// x just above 0.5 and cost 1.4 ulp.
double dblatan(double val)
{
	double ax, u, t, res, v, e;
	int neg, big = 0;
	neg = (val < 0.);
	ax = neg ? -val : val;
	if (ax > 1.) { big = 1; ax = 1. / ax; }
	if (ax <= 0.75)
		res = ax + ax * dblpoly(_poly_atan, 17, ax * ax - 0.28125);
	else
	{
		u = (ax - 1.) / (ax + 1.);
		t = u + u * dblpoly(_poly_atan, 17, u * u - 0.28125);
		dbltwosum(_VAL_PI4_HI, t, &v, &e);
		res = v + (e + _VAL_PI4_LO);
	}
	if (big)
	{
		dbltwosum(_VAL_PI2_HI, -res, &v, &e);
		res = v + (e + _VAL_PI2_LO);
	}
	return neg ? -res : res;
}

double dbllog10(double power)
{
	return dbllog(power) / _ln10;
}

// sinh/cosh/tanh: direct polynomials on |x| <= 1 (which is what keeps tiny |x|
// accurate), then the e^x +- e^-x forms. Above |x| = 1 e^-x is at most 0.37 of
// e^x, so the subtraction never cancels and a single dblexp is enough --
// the earlier t = expm1(|x|) route needed three more roundings to build the
// same quantity.
double dblsinh(double rad)
{
	double ax, z, p, e;
	if (rad != rad) return rad;
	ax = (rad < 0.) ? -rad : rad;
	if (ax <= 1.)
	{
		z = ax * ax;
		p = ax + (ax * z) * dblpoly(_poly_sinh, 6, z);
		return (rad < 0.) ? -p : p;
	}
	if (ax > 710.) return (rad < 0.) ? dblinfn() : dblinfp();
	e = dblexp(ax);
	p = e - 1. / e;
	return (rad < 0.) ? -0.5 * p : 0.5 * p;
}

double dblcosh(double rad)
{
	double ax, z, e;
	if (rad != rad) return rad;
	ax = (rad < 0.) ? -rad : rad;
	if (ax <= 1.)
	{
		z = ax * ax;
		return 1. + z * dblpoly(_poly_cosh, 6, z);
	}
	if (ax > 710.) return dblinfp();
	e = dblexp(ax);
	return 0.5 * (e + 1. / e);
}

double dbltanh(double rad)
{
	double ax, z, p, r;
	if (rad != rad) return rad;
	ax = (rad < 0.) ? -rad : rad;
	if (ax <= 1.)
	{
		z = ax * ax;
		r = ax + (ax * z) * dblpoly(_poly_tanh, 15, z);
		return (rad < 0.) ? -r : r;
	}
	if (ax > 20.) return (rad < 0.) ? -1. : 1.; // 1 - tanh(20) is below 1 ulp
	p = dblexpm1(2. * ax); // e^(2|x|) - 1
	r = 1. - 2. / (p + 2.);
	return (rad < 0.) ? -r : r;
}

double dblasinh(double val)
{
	double ax, t, te, w, res, s;
	ax = (val < 0.) ? -val : val;
	if (ax > 1.)
	{
		w = 1. / ax;
		res = dbllog(ax) + dbllog1p(dblsqrt(1. + w * w));
		return (val < 0.) ? -res : res;
	}
	// e^asinh(|x|) - 1 built from |x|, so every term is positive and the
	// x + sqrt(x^2+1) cancellation of the naive form never happens
	s = dblsqrt(1. + ax * ax);
	dbltwosum(ax, (ax * ax) / (1. + s), &t, &te);
	// te carries the part of t that the addition rounded away; feeding it back
	// through log1p's derivative costs two flops and saves about an ulp
	res = dbllog1p(t) + te / (1. + t);
	return (val < 0.) ? -res : res;
}

double dblacosh(double val)
{
	double t, arg, are;
	if (val < 1.) return dblnanp();
	if (val == 1.) return 0.;
	if (val > 1.0e154) return dbllog(val) + (_VAL_LN2_HI + _VAL_LN2_LO);
	t = val - 1.; // log1p form stays accurate as val -> 1+, and t*t cannot
	dbltwosum(t, dblsqrt(2. * t + t * t), &arg, &are); // overflow below 1e154
	return dbllog1p(arg) + are / (1. + arg); // same rounding recovery as asinh
}

double dblatanh(double val)
{
	double ax;
	ax = (val < 0.) ? -val : val;
	if (ax > 1.) return dblnanp();
	if (ax == 1.) return (val < 0.) ? dblinfn() : dblinfp();
	// 0.5*(log1p(x) - log1p(-x)) == 0.5*log((1+x)/(1-x)); each log1p is well
	// conditioned and the two terms never cancel. Going through 2x/(1-x)
	// instead injects the rounding of the argument into a 1-x that is tiny
	// near |x| = 1, which costs more than a hundred ulp there.
	return 0.5 * (dbllog1p(val) - dbllog1p(-val));
}

