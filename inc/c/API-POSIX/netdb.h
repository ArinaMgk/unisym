// ASCII C TAB4 LF
#ifndef _INC_POSIX_NETDB_H
#define _INC_POSIX_NETDB_H

#include "../stdinc.h"
#include "sys/socket.h"

#define AI_PASSIVE      0x0001
#define AI_CANONNAME    0x0002
#define AI_NUMERICHOST  0x0004
#define AI_NUMERICSERV  0x0008

#define EAI_BADFLAGS    -1
#define EAI_NONAME      -2
#define EAI_AGAIN       -3
#define EAI_FAIL        -4
#define EAI_FAMILY      -6
#define EAI_MEMORY      -10
#define EAI_SERVICE     -8
#define EAI_SOCKTYPE    -7
#define EAI_SYSTEM      -11

struct addrinfo {
	int ai_flags;
	int ai_family;
	int ai_socktype;
	int ai_protocol;
	socklen_t ai_addrlen;
	struct sockaddr* ai_addr;
	char* ai_canonname;
	struct addrinfo* ai_next;
};

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

int getaddrinfo(const char* node, const char* service,
	const struct addrinfo* hints, struct addrinfo** res);
void freeaddrinfo(struct addrinfo* res);
const char* gai_strerror(int error_code);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#endif
