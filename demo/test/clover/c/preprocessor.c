// ASCII C99 TAB4 LF
// Docutitle: Clover C Preprocessor Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#define BASE 3
#define NEXT BASE + 1
#define ADD(left, right) ((left) + (right))
#define TWICE(value) ADD(value, value)
#define MAKE_NAME(left, right) left ## right
#define STRINGIFY(value) #value
#define FIRST(first, ...) first
#define REDIRECT TARGET
#define TARGET(value) value
#define ALICE_HEADER "../../../../inc/c/system/alice.h"

#include ALICE_HEADER
#include "../../../../inc/c/system/alice.h"

#if defined(BASE) && NEXT == 4
	#define SELECTED 10
#elif BASE == 2
	#define SELECTED 20
#else
	#define SELECTED 30
#endif

#undef BASE

#ifndef BASE
	#define EXTRA 2
#else
	#define EXTRA 100
#endif

#if 0 && (1 / 0)
	invalid tokens in an inactive branch
#endif

int main(void) {
	int MAKE_NAME(local, _value) = TWICE(2);
	return SELECTED + EXTRA + local_value +
		FIRST(3, 100) + REDIRECT(5) + STRINGIFY(ok)[0] - 'o' + _BYTE_BITS_;
}
