#ifndef _INC_API_POSIX_PWD
#define _INC_API_POSIX_PWD

#include "../stdinc.h"
#include "api_posix.h"

#ifdef __cplusplus
extern "C" {
#endif

// POSIX.1 <pwd.h>: strings returned by the reentrant queries belong to the caller buffer.
struct passwd {
	char* pw_name;
	char* pw_passwd;
	uid_t pw_uid;
	gid_t pw_gid;
	char* pw_gecos;
	char* pw_dir;
	char* pw_shell;
};

int getpwnam_r(const char* name, struct passwd* entry, char* buffer, size_t size, struct passwd** result);
int getpwuid_r(uid_t uid, struct passwd* entry, char* buffer, size_t size, struct passwd** result);
// Mecocoa prototype: root may select an account; other callers authenticate with the fixed password p.
int UserLogin(const char* name, const char* password);

#ifdef __cplusplus
}
#endif

#endif
