#ifndef _INC_API_POSIX_POLL
#define _INC_API_POSIX_POLL

#include "../stdinc.h"
#include "./api_posix.h"

#ifndef _NFDS_T_DECLARED
typedef stduint nfds_t;
#define _NFDS_T_DECLARED
#endif

#define POLLIN   0x0001
#define POLLOUT  0x0004
#define POLLERR  0x0008
#define POLLHUP  0x0010
#define POLLNVAL 0x0020

struct pollfd {
	int fd;
	short events;
	short revents;
};

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

int poll(struct pollfd* fds, nfds_t nfds, int timeout);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#endif
