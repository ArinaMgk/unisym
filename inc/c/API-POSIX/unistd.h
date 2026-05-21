
#include "../stdinc.h"
#ifndef _INC_API_POSIX_UNISTD
#define _INC_API_POSIX_UNISTD

#include "./api_posix.h"

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

// ---- TASKMAN
	
	/// @brief  exit self
	/// @param status: exit status
	[[noreturn]] void _exit(int status);

	
	/// @brief  clone self
	/// @return parent: return unzero pid of child if successful, -1 if error; child: return zero
	pid_t fork(void);

	/// @brief  get process id
	/// @return: process id
	pid_t getpid(void);

	/// @brief  execute a file
	/// @param  path: path to the executable
	/// @param  argv: argument vector
	/// @param  envp: environment vector
	/// @return -1 if error
	int execve(const char* path, char* const argv[], char* const envp[]);

	/// @brief  execute a file
	/// @param  path: path to the executable
	/// @param  argv: argument vector
	/// @return -1 if error
	int execv(const char* path, char* const argv[]);


// ---- FILEMAN

	// open in fcntl.h

	/// @brief  change current working directory
	/// @param  path: target directory path
	/// @return 0 if successful, -1 if error
	int chdir(const char* path);

	/// @brief  get current working directory
	/// @param  buf: destination buffer
	/// @param  size: size of the buffer
	/// @return pointer to buf if successful, NULL if error
	char* getcwd(char* buf, size_t size);

	/// @brief  close file
	/// @param fd: file descriptor
	/// @return: 0 if successful, -1 if error
	int close(int fd);

	/// @brief  read from file
	/// @param fd: file descriptor
	/// @param buf: buffer
	/// @param nbyte: number of bytes to read
	/// @return: number of bytes read, -1 if error, 0 if EOF
	stdsint read(int fd, void* buf, stduint nbyte);

	/// @brief  write to file
	/// @param fd: file descriptor
	/// @param buf: buffer
	/// @param nbyte: number of bytes to write
	/// @return: number of bytes written, -1 if error
	stdsint write(int fd, const void* buf, size_t nbyte);

// ----



#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif
#endif
