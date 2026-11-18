// ASCII C++ TAB4 LF
// Docutitle: Basic chmod Utility using POSIX API and consio
// Copyright: UNISYM, under Apache License 2.0
#include <sys/stat.h>
#include <c/consio.h>
#include <c/ISO_IEC_STD/errno.h>

// Accept an octal permission value without overflowing mode_t or accepting file-type bits.
static bool parse_mode(const char* text, mode_t& mode) {
	if (!*text) return false;
	mode = 0;
	while (*text) {
		if (*text < '0' || *text > '7' || mode > (07777u - unsigned(*text - '0')) / 8) return false;
		mode = mode * 8 + (*text++ - '0');
	}
	return true;
}

int main(int argc, char* argv[]) {
	int argument = 1;
	if (argument < argc && argv[argument][0] == '-' && argv[argument][1] == '-' && !argv[argument][2]) argument++;
	if (argc - argument < 2) {
		outsfmt("Usage: chmod [--] OCTAL_MODE FILE...\n\r");
		return 1;
	}
	mode_t mode;
	if (!parse_mode(argv[argument], mode)) {
		outsfmt("chmod: invalid octal mode '%s' (expected 0000..7777)\n\r", argv[argument]);
		return 1;
	}
	argument++;
	int failed = 0;
	for (; argument < argc; argument++) {
		if (chmod(argv[argument], mode) < 0) {
			outsfmt("chmod: cannot change mode of '%s' (errno=%d)\n\r", argv[argument], errno);
			failed = 1;
		}
	}
	return failed;
}
