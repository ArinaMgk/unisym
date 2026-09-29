// ASCII C99 TAB4 LF
// Docutitle: Quarter-wave sine table for trigonometric reduction
// Codifiers: @ArinaMgk
// OpLicense: http://unisym.org/license.html

#include "../../../inc/c/auxiliary/trigtab.h"
#include "../../../inc/c/arith.h"

constexpr long double TRIGTAB_PI = _VAL_PI;

// Taylor expansion is locale/ABI independent, safe to be constexpr.
// NOTE: on MSVC long double == double (64-bit); GCC/Clang give 80-bit.
// Either way the final cast to Trigtab truncates, so accuracy is kept.
constexpr long double make_sin(long double x) {
	long double term = x;
	long double sum  = x;
	for (int n = 1; n < 24; ++n) {
		long double a = static_cast<long double>(2 * n);
		long double b = static_cast<long double>(2 * n + 1);
		term *= -x * x / (a * b);
		sum  += term;
	}
	return sum;
}

// Trigtab / TRIGTAB_PI / TRIGTAB_QUARTER_N are settled by the #if in trigtab.h,
// so the MCU(float,N=128) vs host(double,N=256) split is absorbed by the template args.
template <class T, int N>
constexpr uni::Array<T, N + 1> make_sin_quarter() {
	uni::Array<T, N + 1> t{};
	for (int i = 0; i <= N; ++i) {
		long double x = TRIGTAB_PI * static_cast<long double>(i) / (2.0L * static_cast<long double>(N));
		t[i] = static_cast<T>(make_sin(x));
	}
	return t;
}

namespace {
	constexpr auto _tab_sin_quarter_init = make_sin_quarter<Trigtab, TRIGTAB_QUARTER_N>();
}
_ESYM_C const uni::Array<Trigtab, TRIGTAB_QUARTER_N + 1> _tab_sin_quarter = _tab_sin_quarter_init;

static_assert(_tab_sin_quarter_init[0] == static_cast<Trigtab>(0), "sin(0) must be 0");
static_assert(_tab_sin_quarter_init[TRIGTAB_QUARTER_N] > static_cast<Trigtab>(0.999999), "sin(pi/2) must be ~1");