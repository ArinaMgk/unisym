#ifndef _INC_POSIX_SYS_STAT_H
#define _INC_POSIX_SYS_STAT_H

#include "../../stdinc.h"
#include "../../ISO_IEC_STD/time.h"
#include "../api_posix.h"

#define S_IFMT		0170000
#define S_IFDIR		0040000
#define S_IFREG		0100000
#define S_IFCHR		0020000
#define S_IFIFO		0010000
#define S_IFBLK		0060000
#define S_IFLNK		0120000
#define S_IFSOCK		0140000

#define S_ISUID		0004000
#define S_ISGID		0002000
#define S_ISVTX		0001000

#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

struct stat {
	stduint st_size;
	stduint st_mode;
	time_t st_mtime;
	uid_t st_uid;
	gid_t st_gid;
};

typedef stduint mode_t;

int fstat(int fd, struct stat *buf);
int stat(const char *path, struct stat *buf);
int chmod(const char* path, mode_t mode);
int fchmod(int fd, mode_t mode);
int mkdir(const char *path, mode_t mode);
mode_t umask(mode_t mask);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#endif
