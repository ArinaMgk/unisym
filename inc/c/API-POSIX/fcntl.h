
#include "../stdinc.h"
#ifndef _INC_API_POSIX_FCNTL
#define _INC_API_POSIX_FCNTL

#include "./api_posix.h"

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

	// POSIX.1-2008 standard flags
	#define O_RDONLY    0
	#define O_WRONLY    1
	#define O_RDWR      2
	#define O_ACCMODE   3

	#define O_CREAT     0x40
	#define O_EXCL      0x80
	#define O_NOCTTY    0x100
	#define O_TRUNC     0x200
	#define O_APPEND    0x400
	#define O_NONBLOCK  0x800
	#define O_DIRECTORY 0x10000

	/// @brief  open file
	/// @param  path: file path
	/// @param  oflag: access mode and flags
	/// @param  ...: optional mode (if O_CREAT)
	/// @return file descriptor, -1 if error
	int open(const char* path, int oflag, ...);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif
#endif
