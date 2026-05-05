
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
	pid_t wait(int* status);

	typedef enum {
		P_ALL,
		P_PID,
		P_PGID
	} idtype_t;

	typedef struct {
		pid_t si_pid;
		int   si_status;
		int   si_code;
	} siginfo_t;

	// Options for waitid
	#define WEXITED   0x00000004
	#define WSTOPPED  0x00000008
	#define WCONTINUED 0x00000010
	#define WNOHANG   0x00000001
	#define WNOWAIT   0x00000100

	/// @brief  wait for subprocess to terminate
	/// @param  idtype: P_ALL, P_PID, or P_PGID
	/// @param  id: pid or pgid
	/// @param  infop: `OUT` pointer to siginfo_t
	/// @param  options: WEXITED, etc.
	/// @return 0 if successful, -1 if error
	int waitid(idtype_t idtype, id_t id, siginfo_t* infop, int options);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif
#endif
