// ASCII C++ TAB4 CRLF
// Docutitle: (Module) Basic pwd Utility using POSIX API and consio
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Hostedstanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include <unistd.h>
#include <c/consio.h>

int main(int argc, char *argv[]) {
	char buf[512];
	if (getcwd(buf, sizeof(buf))) {
		// Print absolute path with newline
		outsfmt("%s\n", buf);
		return 0;
	} else {
		outsfmt("pwd: failed to get current directory\n");
		return 1;
	}
}
