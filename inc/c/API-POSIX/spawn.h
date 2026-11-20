#include "../stdinc.h"
#ifndef _INC_API_POSIX_SPAWN
#define _INC_API_POSIX_SPAWN

#include "./api_posix.h"
#include "./sys/stat.h"
#include "./sched.h"
#include "../ISO_IEC_STD/signal.h"

#if !defined(_SIGSET_T_DEFINED)
typedef _POSIX_sigset_t sigset_t;
#define _SIGSET_T_DEFINED
#endif

#if defined(_INC_CPP) || defined(__cplusplus)
extern "C" {
#endif

typedef struct {
	void* __actions;
	size_t __count;
	size_t __capacity;
} posix_spawn_file_actions_t;

typedef struct {
	short __flags;
	pid_t __pgroup;
	sigset_t __sigdefault;
	sigset_t __sigmask;
	int __schedpolicy;
	int __schedpriority;
} posix_spawnattr_t;

#define POSIX_SPAWN_RESETIDS		0x01
#define POSIX_SPAWN_SETPGROUP		0x02
#define POSIX_SPAWN_SETSIGDEF		0x04
#define POSIX_SPAWN_SETSIGMASK		0x08
#define POSIX_SPAWN_SETSCHEDPARAM	0x10
#define POSIX_SPAWN_SETSCHEDULER	0x20

int posix_spawn(pid_t* pid, const char* path,
	const posix_spawn_file_actions_t* file_actions,
	const posix_spawnattr_t* attrp,
	char* const argv[], char* const envp[]);
int posix_spawnp(pid_t* pid, const char* file,
	const posix_spawn_file_actions_t* file_actions,
	const posix_spawnattr_t* attrp,
	char* const argv[], char* const envp[]);

int posix_spawn_file_actions_init(posix_spawn_file_actions_t* file_actions);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t* file_actions);
int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t* file_actions,
	int fd, const char* path, int oflag, mode_t mode);
int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t* file_actions, int fd);
int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t* file_actions,
	int fd, int newfd);

int posix_spawnattr_init(posix_spawnattr_t* attr);
int posix_spawnattr_destroy(posix_spawnattr_t* attr);
int posix_spawnattr_getflags(const posix_spawnattr_t* attr, short* flags);
int posix_spawnattr_setflags(posix_spawnattr_t* attr, short flags);
int posix_spawnattr_getpgroup(const posix_spawnattr_t* attr, pid_t* pgroup);
int posix_spawnattr_setpgroup(posix_spawnattr_t* attr, pid_t pgroup);
int posix_spawnattr_getsigdefault(const posix_spawnattr_t* attr,
	sigset_t* sigdefault);
int posix_spawnattr_setsigdefault(posix_spawnattr_t* attr,
	const sigset_t* sigdefault);
int posix_spawnattr_getsigmask(const posix_spawnattr_t* attr,
	sigset_t* sigmask);
int posix_spawnattr_setsigmask(posix_spawnattr_t* attr,
	const sigset_t* sigmask);
int posix_spawnattr_getschedpolicy(const posix_spawnattr_t* attr,
	int* schedpolicy);
int posix_spawnattr_setschedpolicy(posix_spawnattr_t* attr, int schedpolicy);
int posix_spawnattr_getschedparam(const posix_spawnattr_t* attr,
	struct sched_param* schedparam);
int posix_spawnattr_setschedparam(posix_spawnattr_t* attr,
	const struct sched_param* schedparam);

#if defined(_INC_CPP) || defined(__cplusplus)
}
#endif
#endif
