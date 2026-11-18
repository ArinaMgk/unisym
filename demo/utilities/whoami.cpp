// ASCII C++ TAB4 CRLF
// Docutitle: Current user name utility
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <c/consio.h>

int main(int argc, char* argv[]) {
	if (argc != 1) {
		outsfmt("Usage: whoami\n\r");
		return 1;
	}
	const uid_t uid = geteuid();
	if (uid < 0) return 1;
	char* buffer = (char*)malloc(256);
	if (!buffer) return 1;
	struct passwd entry{};
	struct passwd* found = nullptr;
	const int status = getpwuid_r(uid, &entry, buffer, 256, &found);
	if (!status && found) outsfmt("%s\n\r", entry.pw_name);
	else outsfmt("whoami: no user name for uid=%lu (error=%d)\n\r", (unsigned long)uid, status);
	free(buffer);
	return status || !found ? 1 : 0;
}
