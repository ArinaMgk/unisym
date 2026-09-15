#ifndef _INC_POSIX_ARPA_INET_H
#define _INC_POSIX_ARPA_INET_H

#include "../netinet/in.h"

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

in_addr_t inet_addr(const char* cp);
int inet_aton(const char* cp, struct in_addr* inp);
char* inet_ntoa(struct in_addr in);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#endif
