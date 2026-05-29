#ifndef _INC_STDARG_NEXT
#define _INC_STDARG_NEXT
#if defined(__has_include_next)
#	if __has_include_next(<stdarg.h>)
#		include_next <stdarg.h>
#	endif
#elif defined(__GNUC__) || defined(__clang__)
#include_next <stdarg.h>
#endif
#endif

#include "../stdinc.h" // supple

#if !defined(_STDARG_H) && !defined(_STDARG_H_) && !defined(_ANSI_STDARG_H_)
#define va_list para_list
#define __va_align(type) _para_align(type)
#define va_start para_ento
#define va_arg para_next
#define va_end para_endo
#define va_copy para_copy
#endif