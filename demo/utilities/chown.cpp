// ASCII C++ TAB4 LF
// Docutitle: Basic chown Utility using POSIX API and consio
// Copyright: UNISYM, under Apache License 2.0
#include <unistd.h>
#include <c/consio.h>
#include <c/ISO_IEC_STD/errno.h>

// Numeric identities match the kernel's supported nonnegative ID range.
static bool parse_id(const char*& text, stdsint& id) {
	if (*text < '0' || *text > '9') return false;
	id = 0;
	while (*text >= '0' && *text <= '9') {
		const unsigned digit = unsigned(*text - '0');
		if (id > (0x7FFFFFFF - digit) / 10) return false;
		id = id * 10 + digit;
		text++;
	}
	return true;
}

int main(int argc, char* argv[]) {
	int argument = 1;
	if (argument < argc && argv[argument][0] == '-' && argv[argument][1] == '-' && !argv[argument][2]) argument++;
	if (argc - argument < 2) {
		outsfmt("Usage: chown [--] UID[:GID] FILE... | chown [--] :GID FILE...\n\r");
		return 1;
	}
	const char* text = argv[argument];
	stdsint owner = -1;
	stdsint group = -1;
	bool valid = true;
	if (*text != ':') valid = parse_id(text, owner);
	if (valid && *text == ':') {
		text++;
		valid = parse_id(text, group);
	}
	if (!valid || *text || (owner == -1 && group == -1)) {
		outsfmt("chown: invalid numeric owner/group '%s'\n\r", argv[argument]);
		return 1;
	}
	int failed = 0;
	for (argument++; argument < argc; argument++) {
		if (chown(argv[argument], uid_t(owner), gid_t(group)) < 0) {
			outsfmt("chown: cannot change ownership of '%s' (errno=%d)\n\r", argv[argument], errno);
			failed = 1;
		}
	}
	return failed;
}
