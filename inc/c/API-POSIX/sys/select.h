#ifndef _INC_API_POSIX_SYS_SELECT
#define _INC_API_POSIX_SYS_SELECT

#include "../../stdinc.h"
#include "../api_posix.h"

#ifndef FD_SETSIZE
#define FD_SETSIZE 64
#endif

#ifndef _FD_MASK_DECLARED
typedef stduint fd_mask;
#define _FD_MASK_DECLARED
#endif

#define __FD_MASK_BITS (sizeof(fd_mask) * 8)
#define __FD_SET_WORD(fd) ((fd) / __FD_MASK_BITS)
#define __FD_SET_MASK(fd) ((fd_mask)1 << ((fd) % __FD_MASK_BITS))

typedef struct fd_set {
	fd_mask fds_bits[(FD_SETSIZE + __FD_MASK_BITS - 1) / __FD_MASK_BITS];
} fd_set;

struct timeval {
	stduint tv_sec;
	stduint tv_usec;
};

#define FD_ZERO(set) do { \
	stduint __fd_i; \
	for (__fd_i = 0; __fd_i < (sizeof((set)->fds_bits) / sizeof((set)->fds_bits[0])); __fd_i++) { \
		(set)->fds_bits[__fd_i] = 0; \
	} \
} while (0)

#define FD_SET(fd, set) do { \
	if ((fd) >= 0 && (fd) < FD_SETSIZE) { \
		(set)->fds_bits[__FD_SET_WORD(fd)] |= __FD_SET_MASK(fd); \
	} \
} while (0)

#define FD_CLR(fd, set) do { \
	if ((fd) >= 0 && (fd) < FD_SETSIZE) { \
		(set)->fds_bits[__FD_SET_WORD(fd)] &= ~__FD_SET_MASK(fd); \
	} \
} while (0)

#define FD_ISSET(fd, set) \
	((fd) >= 0 && (fd) < FD_SETSIZE && (((set)->fds_bits[__FD_SET_WORD(fd)] & __FD_SET_MASK(fd)) != 0))

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#endif
