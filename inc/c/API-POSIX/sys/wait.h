
#include "../stdinc.h"
#ifndef _INC_API_POSIX_SYS_WAIT
#define _INC_API_POSIX_SYS_WAIT

#include "../api_posix.h"

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

	/// @brief  wait for subprocess to terminate
	/// @param  status: `OUT` pointer to status variable
	/// @return pid of terminated subprocess
	pid_t _Comment(pid) wait(int* status);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif
#endif
