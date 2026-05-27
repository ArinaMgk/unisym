#ifndef _INC_SETJMP
#define _INC_SETJMP

#include "../stdinc.h" // supple

#ifdef __cplusplus
extern "C" {
#endif

typedef Retpoint jmp_buf;

#define setjmp(buf) ((int)MarkPoint(&(buf)))
#define longjmp(buf, val) JumpPoint(&(buf), (pureptr_t)(stduint)(val))

#ifdef __cplusplus
}
#endif

#endif /* _INC_SETJMP */
