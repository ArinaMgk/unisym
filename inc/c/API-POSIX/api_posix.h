#ifndef _INC_API_POSIX
#define _INC_API_POSIX

#ifndef pid_t
#define pid_t stdsint
#endif
#ifndef id_t
#define id_t  stdsint
#endif
#ifndef uid_t
#define uid_t stdsint
#endif
#if !defined(_OFF_T_DECLARED) && !defined(__off_t_defined) && !defined(_OFF_T_DEFINED)
typedef stdsint off_t;
#define _OFF_T_DECLARED
#define __off_t_defined
#define _OFF_T_DEFINED
#endif



#endif
