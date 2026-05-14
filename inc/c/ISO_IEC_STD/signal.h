// UTF-8 C99 TAB4 CRLF
// Docutitle: Signal
// Codifiers: @dosconio: 20240513
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INC_SIGNAL
#define _INC_SIGNAL

#include "../stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef pid_t
#define pid_t stdsint	// Process ID type
#endif
#ifndef uid_t
#define uid_t stdsint	// User ID type
#endif

typedef int sig_atomic_t;
typedef void (*__sighandler_t)(int);

#define SIG_DFL ((__sighandler_t)0)	// Default action
#define SIG_IGN ((__sighandler_t)1)	// Ignore signal
#define SIG_ERR ((__sighandler_t)-1)	// Error return

// Signal numbers
enum Signal {
	// Standard Signals (1-31)
	SIGHUP		= 1,	// Term: Hangup
	SIGINT		= 2,	// Term: Interrupt
	SIGQUIT		= 3,	// Core: Quit
	SIGILL		= 4,	// Core: Illegal Instruction
	SIGTRAP		= 5,	// Core: Trace/breakpoint trap
	SIGABRT		= 6,	// Core: Abort
	SIGIOT		= 6,	// Core: IOT trap
	SIGBUS		= 7,	// Core: Bus error
	SIGFPE		= 8,	// Core: Floating point exception
	SIGKILL		= 9,	// Term: Kill (Unblockable)
	SIGUSR1		= 10,	// Term: User-defined signal 1
	SIGSEGV		= 11,	// Core: Invalid memory reference
	SIGUSR2		= 12,	// Term: User-defined signal 2
	SIGPIPE		= 13,	// Term: Broken pipe
	SIGALRM		= 14,	// Term: Alarm clock
	SIGTERM		= 15,	// Term: Termination
	SIGSTKFLT	= 16,	// Term: Stack fault
	SIGCHLD		= 17,	// Ign:  Child stopped or terminated
	SIGCONT		= 18,	// Cont: Continue if stopped
	SIGSTOP		= 19,	// Stop: Stop process (Unblockable)
	SIGTSTP		= 20,	// Stop: Keyboard stop
	SIGTTIN		= 21,	// Stop: Background read from tty
	SIGTTOU		= 22,	// Stop: Background write to tty
	SIGURG		= 23,	// Ign:  Urgent condition on socket
	SIGXCPU		= 24,	// Core: CPU limit exceeded
	SIGXFSZ		= 25,	// Core: File size limit exceeded
	SIGVTALRM	= 26,	// Term: Virtual alarm clock
	SIGPROF		= 27,	// Term: Profiling alarm clock
	SIGWINCH	= 28,	// Ign:  Window size change
	SIGIO		= 29,	// Term: I/O now possible
	SIGPOLL		= SIGIO,
	SIGPWR		= 30,	// Term: Power failure restart
	SIGSYS		= 31,	// Core: Bad system call
	SIGUNUSED	= 31,

	// Real-time Signals (32-64)
	SIGRTMIN	= 32,
	SIGRTMAX	= 64
};

#define _NSIG 65

#if !defined(__sigset_t_defined) && !defined(_SIGSET_T_DEFINED)
#define _SIGSET_T_DEFINED
// Signal sets
typedef struct {
	uint64_t sig[1];
} sigset_t;
#endif

// Internal helper to access the first 64 bits of a signal set
#define _sigset_raw(set) (*(uint64_t*)(set))

#ifndef _SIGINFO_T_DEFINED
#define _SIGINFO_T_DEFINED
// Signal information structure
typedef struct {
	int      si_signo;	// Signal number
	int      si_errno;	// An errno value
	int      si_code;	// Signal code
	pid_t    si_pid;	// Sending process ID
	uid_t    si_uid;	// Real user ID of sending process
	void*    si_addr;	// Memory location which caused fault
	int      si_status;	// Exit value or signal
	union {
		int _pad[12];
	} _sifields;
} siginfo_t;
#endif

// sigaction structure
struct sigaction {
	union {
		__sighandler_t sa_handler;
		void (*sa_sigaction)(int, siginfo_t*, void*);
	} __sigaction_handler;
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_restorer)(void);
};

#define sa_handler   __sigaction_handler.sa_handler
#define sa_sigaction __sigaction_handler.sa_sigaction

// sa_flags bits
#define SA_NOCLDSTOP	1
#define SA_NOCLDWAIT	2
#define SA_SIGINFO		4
#define SA_ONSTACK		0x08000000
#define SA_RESTART		0x10000000
#define SA_NODEFER		0x40000000
#define SA_RESETHAND	0x80000000

// Signal functions
__sighandler_t signal(int sig, __sighandler_t handler);
int sigaction(int sig, const struct sigaction* act, struct sigaction* oact);
int kill(pid_t pid, int sig);
int sigemptyset(sigset_t* set);
int sigfillset(sigset_t* set);
int sigaddset(sigset_t* set, int signo);
int sigdelset(sigset_t* set, int signo);
int sigismember(const sigset_t* set, int signo);

#ifdef __cplusplus
}
#endif

#endif // _INC_SIGNAL