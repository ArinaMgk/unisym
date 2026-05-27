// ASCII C TAB4 CRLF
// Docutitle: (Module) Basic mkdir Utility using POSIX API and consio
// Codifiers: @Antigravity
// Attribute: Arn-Covenant Any-Architect Env-Hostedstanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include <unistd.h>
#include <sys/stat.h>
#include <c/consio.h>

int main(int argc, char *argv[]) {
	// Require at least one directory name argument
	if (argc < 2) {
		outsfmt("mkdir: missing operand\n\r");
		return 1;
	}

	int has_error = 0;
	for (int i = 1; i < argc; i++) {
		// Invoke POSIX mkdir wrapper (O_CREAT | O_DIRECTORY)
		if (mkdir(argv[i], 0777) < 0) {
			outsfmt("mkdir: cannot create directory '%s'\n\r", argv[i]);
			has_error = 1;
		}
	}

	return has_error ? 1 : 0;
}
