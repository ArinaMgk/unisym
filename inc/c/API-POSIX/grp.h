#ifndef _INC_API_POSIX_GRP
#define _INC_API_POSIX_GRP

#include "../stdinc.h"
#include "api_posix.h"

#ifdef __cplusplus
extern "C" {
#endif

// POSIX.1 <grp.h>: supplementary membership is not implemented in the single-primary-group model.
struct group {
	char* gr_name;
	char* gr_passwd;
	gid_t gr_gid;
	char** gr_mem;
};

int getgrnam_r(const char* name, struct group* entry, char* buffer, size_t size, struct group** result);
int getgrgid_r(gid_t gid, struct group* entry, char* buffer, size_t size, struct group** result);

#ifdef __cplusplus
}
#endif

#endif
