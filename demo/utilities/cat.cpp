// ASCII C++ TAB4 CRLF
// Docutitle: (Module) Basic cat Utility using POSIX API and consio
// Codifiers: @Antigravity
// Attribute: Arn-Covenant Any-Architect Env-Hostedstanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include <unistd.h>
#include <fcntl.h>
#include <c/consio.h>

// Helper to copy from fd to stdout (1)
static bool copy_fd_to_stdout(int fd) {
	char buffer[1024];
	while (true) {
		stdsint bytes_read = read(fd, buffer, sizeof(buffer));
		if (bytes_read < 0) {
			return false;
		}
		if (bytes_read == 0) {
			break;
		}
		stduint total_written = 0;
		while (total_written < (stduint)bytes_read) {
			stdsint bytes_written = write(1, buffer + total_written, bytes_read - total_written);
			if (bytes_written < 0) {
				return false;
			}
			total_written += bytes_written;
		}
	}
	return true;
}

int main(int argc, char *argv[]) {
	bool has_error = false;
	
	// If no arguments, read from stdin
	if (argc < 2) {
		if (!copy_fd_to_stdout(0)) {
			outsfmt("cat: error reading stdin\n\r");
			return 1;
		}
		return 0;
	}

	for (int i = 1; i < argc; i++) {
		const char* arg = argv[i];
		// If argument is "-", read from stdin
		if (arg[0] == '-' && arg[1] == '\0') {
			if (!copy_fd_to_stdout(0)) {
				outsfmt("cat: error reading stdin\n\r");
				has_error = true;
			}
		} else {
			// Otherwise open the file
			int fd = open(arg, O_RDONLY);
			if (fd < 0) {
				outsfmt("cat: %s: No such file or directory\n\r", arg);
				has_error = true;
				continue;
			}
			if (!copy_fd_to_stdout(fd)) {
				outsfmt("cat: %s: error reading file\n\r", arg);
				has_error = true;
			}
			close(fd);
		}
	}

	return has_error ? 1 : 0;
}
