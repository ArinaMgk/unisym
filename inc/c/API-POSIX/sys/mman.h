#ifndef _INC_POSIX_SYS_MMAN_H
#define _INC_POSIX_SYS_MMAN_H

#include "../../stdinc.h"
#include "../api_posix.h"

#ifndef _OFF_T_DECLARED
typedef stduint off_t;
#define _OFF_T_DECLARED
#endif

// Memory protection attributes
#define PROT_NONE	0x0
#define PROT_READ	0x1 // present
#define PROT_WRITE	0x2 // writable
#define PROT_EXEC	0x4 // user

// Map attributes
#define MAP_SHARED	0x01
#define MAP_PRIVATE	0x02
#define MAP_ANONYMOUS	0x20

#define MAP_FAILED	((void*)~_IMM0)

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void* addr, size_t length);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#endif // _INC_POSIX_SYS_MMAN_H
