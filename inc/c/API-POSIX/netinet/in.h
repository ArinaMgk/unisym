#ifndef _INC_POSIX_NETINET_IN_H
#define _INC_POSIX_NETINET_IN_H

#include "../../stdinc.h"
#include "../../ustring.h"
#include "../api_posix.h"
#include "../sys/socket.h"

#ifndef _IN_PORT_T_DECLARED
typedef uint16 in_port_t;
#define _IN_PORT_T_DECLARED
#endif

#ifndef _IN_ADDR_T_DECLARED
typedef uint32 in_addr_t;
#define _IN_ADDR_T_DECLARED
#endif

#define INADDR_ANY         ((in_addr_t)0x00000000u)
#define INADDR_BROADCAST   ((in_addr_t)0xFFFFFFFFu)
#define INADDR_LOOPBACK    ((in_addr_t)0x7F000001u)

#define IPPROTO_IP         0
#define IPPROTO_ICMP       1
#define IPPROTO_TCP        6
#define IPPROTO_UDP        17
#define IPPROTO_ICMPV6     58

struct in_addr {
	in_addr_t s_addr;
};

struct sockaddr_in {
	sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
	unsigned char sin_zero[8];
};

struct in6_addr {
	unsigned char s6_addr[16];
};

struct sockaddr_in6 {
	sa_family_t sin6_family;
	in_port_t sin6_port;
	uint32 sin6_flowinfo;
	struct in6_addr sin6_addr;
	uint32 sin6_scope_id;
};

static inline uint16 htons(uint16 host16) {
	#if defined(_INC_CPP) || defined(__cplusplus)
	return MemReverseB(host16);
	#elif __ENDIAN__
	return host16;
	#else
	return (uint16)((host16 << 8) | (host16 >> 8));
	#endif
}

static inline uint16 ntohs(uint16 net16) {
	return htons(net16);
}

static inline uint32 htonl(uint32 host32) {
	#if defined(_INC_CPP) || defined(__cplusplus)
	return MemReverseB(host32);
	#elif __ENDIAN__
	return host32;
	#else
	return ((host32 & 0x000000FFu) << 24) |
		((host32 & 0x0000FF00u) << 8) |
		((host32 & 0x00FF0000u) >> 8) |
		((host32 & 0xFF000000u) >> 24);
	#endif
}

static inline uint32 ntohl(uint32 net32) {
	return htonl(net32);
}

#endif
