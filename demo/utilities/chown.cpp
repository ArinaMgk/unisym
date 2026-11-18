// ASCII C++ TAB4 LF
// Docutitle: Basic chown Utility using POSIX API and consio
// Copyright: UNISYM, under Apache License 2.0
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
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

// All-digit selectors remain decimal IDs, including IDs without an account record.
static int ResolveIdentity(const char* name, bool is_group, stdsint& id) {
	if (!*name) return EINVAL;
	bool numeric = true;
	for (const char* p = name; *p; p++) if (*p < '0' || *p > '9') numeric = false;
	if (numeric) {
		const char* text = name;
		return parse_id(text, id) && !*text ? 0 : EINVAL;
	}
	size_t capacity = 256;
	while (true) {
		char* buffer = (char*)malloc(capacity);
		if (!buffer) return ENOMEM;
		int status;
		if (is_group) {
			struct group entry{};
			struct group* found = nullptr;
			status = getgrnam_r(name, &entry, buffer, capacity, &found);
			if (!status && found) id = entry.gr_gid;
			else if (!status) status = ENOENT;
		} else {
			struct passwd entry{};
			struct passwd* found = nullptr;
			status = getpwnam_r(name, &entry, buffer, capacity, &found);
			if (!status && found) id = entry.pw_uid;
			else if (!status) status = ENOENT;
		}
		free(buffer);
		if (!status && (id < 0 || uint64(id) > 0x7FFFFFFFu)) return EINVAL;
		if (status != ERANGE) return status;
		// Reentrant account queries may need more space as home and shell records grow.
		if (capacity > size_t(-1) / 2) return ERANGE;
		capacity *= 2;
	}
}

int main(int argc, char* argv[]) {
	int argument = 1;
	if (argument < argc && argv[argument][0] == '-' && argv[argument][1] == '-' && !argv[argument][2]) argument++;
	if (argc - argument < 2) {
		outsfmt("Usage: chown [--] OWNER[:GROUP] FILE... | chown [--] :GROUP FILE...\n\r");
		return 1;
	}
	const char* selector = argv[argument];
	const size_t length = strlen(selector);
	char* names = (char*)malloc(length + 1);
	if (!names) {
		outsfmt("chown: cannot allocate owner/group storage\n\r");
		return 1;
	}
	memcpy(names, selector, length + 1);
	char* group_name = strchr(names, ':');
	if (group_name) *group_name++ = 0;
	// Resolve the whole selector before changing any file; an empty owner keeps the current owner.
	if ((!*names && !group_name) || (group_name && (!*group_name || strchr(group_name, ':')))) {
		outsfmt("chown: invalid owner/group '%s'\n\r", selector);
		free(names);
		return 1;
	}
	stdsint owner = -1;
	stdsint group = -1;
	int status = *names ? ResolveIdentity(names, false, owner) : 0;
	if (status) {
		outsfmt("chown: cannot resolve user '%s' (error=%d)\n\r", names, status);
		free(names);
		return 1;
	}
	if (group_name) status = ResolveIdentity(group_name, true, group);
	if (status) {
		outsfmt("chown: cannot resolve group '%s' (error=%d)\n\r", group_name, status);
		free(names);
		return 1;
	}
	free(names);
	int failed = 0;
	for (argument++; argument < argc; argument++) {
		if (chown(argv[argument], uid_t(owner), gid_t(group)) < 0) {
			outsfmt("chown: cannot change ownership of '%s' (errno=%d)\n\r", argv[argument], errno);
			failed = 1;
		}
	}
	return failed;
}
