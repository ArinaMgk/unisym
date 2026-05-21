#ifndef _INC_POSIX_SYS_STAT_H
#define _INC_POSIX_SYS_STAT_H

#include "../../stdinc.h"
#include "../api_posix.h"

#define S_IFMT		0170000
#define S_IFDIR		0040000
#define S_IFREG		0100000
#define S_IFCHR		0020000

#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

struct stat {
	stduint st_size;
	stduint st_mode;
};

int fstat(int fd, struct stat *buf);
int stat(const char *path, struct stat *buf);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#endif
