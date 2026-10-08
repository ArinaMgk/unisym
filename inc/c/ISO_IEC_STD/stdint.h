// since ISO/IEC-C99
// ARM32 has no architect header, so the integer types come from the toolchain
#if (defined(_OPT_ARM32) || (!defined(_ACCM) && !defined(_MCCA))) && defined(__has_include_next)
#	if __has_include_next(<stdint.h>)
#		include_next <stdint.h>
#	endif
#elif (defined(_OPT_ARM32) || (!defined(_ACCM) && !defined(_MCCA))) && (defined(__GNUC__) || defined(__clang__))
#include_next <stdint.h>
#endif
#include "../datype/integer.h"
