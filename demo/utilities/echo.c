// ASCII C TAB4 CRLF
// Docutitle: (Module) Basic echo Utility using POSIX API and consio
// Codifiers: @Antigravity
// Attribute: Arn-Covenant Any-Architect Env-Hostedstanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include <unistd.h>
#include <c/consio.h>

int main(int argc, char *argv[]) {
	int start_idx = 1;
	int newline = 1;

	// Check for the -n option to suppress the trailing newline
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'n' && argv[1][2] == '\0') {
		newline = 0;
		start_idx = 2;
	}

	for (int i = start_idx; i < argc; i++) {
		outsfmt("%s", argv[i]);
		if (i < argc - 1) {
			outsfmt(" ");
		}
	}

	if (newline) {
		outsfmt("\n\r");
	}

	return 0;
}
