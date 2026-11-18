// ASCII C++ TAB4 CRLF
// Docutitle: Switch to a built-in account and replace this command with its shell
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <c/consio.h>
#include <c/ISO_IEC_STD/errno.h>
#include <cpp/System/Terminal/Backend.hpp>

// Build a fresh shell environment instead of inheriting privileged loader and startup variables.
static char* MakeEnvironment(const char* name, const char* value) {
	const size_t prefix = strlen(name);
	const size_t length = strlen(value);
	char* result = (char*)malloc(prefix + length + 2);
	if (!result) return nullptr;
	memcpy(result, name, prefix);
	result[prefix] = '=';
	memcpy(result + prefix + 1, value, length + 1);
	return result;
}

// Read raw terminal events without echoing; reject an oversized password after consuming the line.
static bool ReadPassword(char* password, size_t capacity) {
	if (!password || capacity < 2) return false;
	uni::TerminalBackend backend;
	uni::TerminalRaw raw{};
	size_t length = 0;
	bool overflow = false;
	bool complete = false;
	password[0] = 0;

	while (backend.Next(raw)) {
		if (raw.properties == uni::TerminalRawProperties::Release) continue;
		const byte ch = raw.character;
		if (ch == '\r' || ch == '\n') {
			complete = !overflow;
			break;
		}
		if (ch == 0x03 || ch == 0x04) break;
		if (overflow) continue;
		if (ch == '\b' || ch == 0x7F) {
			if (length) password[--length] = 0;
			continue;
		}
		if (ch < 0x20) continue;
		if (length + 1 >= capacity) {
			overflow = true;
			continue;
		}
		password[length++] = char(ch);
		password[length] = 0;
	}

	write(1, "\n\r", 2);
	if (!complete) {
		for (size_t i = 0; i < capacity; i++) ((volatile char*)password)[i] = 0;
	}
	return complete;
}

int main(int argc, char* argv[]) {
	if (argc > 2 || (argc == 2 && argv[1][0] == '-')) {
		outsfmt("Usage: su [USER]\n\r");
		return 1;
	}
	const char* name = argc == 2 ? argv[1] : "root";
	char* buffer = (char*)malloc(256);
	if (!buffer) return 1;
	struct passwd user{};
	struct passwd* found = nullptr;
	const int status = getpwnam_r(name, &user, buffer, 256, &found);
	if (status || !found) {
		outsfmt("su: unknown user '%s' (error=%d)\n\r", name, status);
		free(buffer);
		return 1;
	}
	char password[16]{};
	if (getuid() != 0) {
		outsfmt("Password: ");
		if (!ReadPassword(password, sizeof(password))) {
			free(buffer);
			return 1;
		}
	}
	const int authenticated = UserLogin(user.pw_name, password);
	for (volatile char* p = password; p != password + sizeof(password); ++p) *p = 0;
	if (authenticated < 0) {
		outsfmt("su: authentication failed (errno=%d)\n\r", errno);
		free(buffer);
		return 1;
	}
	// Preserve the configured application directories, including the runtime root apps directory.
	const char* path = getenv("PATH");
	char* environment[] = {
		MakeEnvironment("USER", user.pw_name),
		MakeEnvironment("LOGNAME", user.pw_name),
		MakeEnvironment("HOME", user.pw_dir),
		MakeEnvironment("SHELL", user.pw_shell),
		MakeEnvironment("PATH", path && *path ? path : "/md0"),
		nullptr,
	};
	bool ready = true;
	for (unsigned i = 0; i < 5; i++) if (!environment[i]) ready = false;
	if (!ready) errno = ENOMEM;
	else if (chdir(user.pw_dir) == 0) {
		char* shell_arguments[] = { user.pw_shell, nullptr };
		// This command is already a child of its caller; successful exec keeps the authenticated identity.
		execve(user.pw_shell, shell_arguments, environment);
	}
	outsfmt("su: cannot start shell for '%s' (errno=%d)\n\r", name, errno);
	for (unsigned i = 0; i < 5; i++) free(environment[i]);
	free(buffer);
	return 1;
}
