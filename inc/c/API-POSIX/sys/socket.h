#ifndef _INC_POSIX_SYS_SOCKET_H
#define _INC_POSIX_SYS_SOCKET_H

#include "../../stdinc.h"
#include "../api_posix.h"

#ifndef _SA_FAMILY_T_DECLARED
typedef uint16 sa_family_t;
#define _SA_FAMILY_T_DECLARED
#endif

#ifndef _SOCKLEN_T_DECLARED
typedef stduint socklen_t;
#define _SOCKLEN_T_DECLARED
#endif

#define AF_UNSPEC   0
#define AF_INET     2
#define AF_INET6    10
#define AF_PACKET   17

#define PF_UNSPEC   AF_UNSPEC
#define PF_INET     AF_INET
#define PF_INET6    AF_INET6
#define PF_PACKET   AF_PACKET

#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

#define SHUT_RD     0
#define SHUT_WR     1
#define SHUT_RDWR   2

struct sockaddr {
	sa_family_t sa_family;
	char sa_data[14];
};

struct sockaddr_storage {
	sa_family_t ss_family;
	char ss_data[126];
};

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr* address, socklen_t address_length);
int connect(int sockfd, const struct sockaddr* address, socklen_t address_length);
stdsint send(int sockfd, const void* buffer, size_t length, int flags);
stdsint recv(int sockfd, void* buffer, size_t length, int flags);
stdsint sendto(int sockfd, const void* buffer, size_t length, int flags,
	const struct sockaddr* address, socklen_t address_length);
stdsint recvfrom(int sockfd, void* buffer, size_t length, int flags,
	struct sockaddr* address, socklen_t* address_length);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr* address, socklen_t* address_length);
int shutdown(int sockfd, int how);
int getsockname(int sockfd, struct sockaddr* address, socklen_t* address_length);
int getpeername(int sockfd, struct sockaddr* address, socklen_t* address_length);
int setsockopt(int sockfd, int level, int option_name, const void* option_value, socklen_t option_length);
int getsockopt(int sockfd, int level, int option_name, void* option_value, socklen_t* option_length);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#endif
