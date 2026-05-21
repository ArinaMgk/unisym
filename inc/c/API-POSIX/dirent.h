#ifndef _INC_POSIX_DIRENT_H
#define _INC_POSIX_DIRENT_H

#include "../stdinc.h"
#include "api_posix.h"

#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

struct dirent {
	stduint d_ino;			// Inode number
	stduint d_off;			// Offset
	unsigned short d_reclen;	// Record length
	unsigned char d_type;		// Type of file
	char d_name[256];		// File name
};

typedef struct {
	int fd;
	struct dirent current_entry;
} DIR;

DIR *opendir(const char *name);
DIR *fdopendir(int fd);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif

#endif
