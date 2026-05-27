// ASCII C TAB4 CRLF
// Docutitle: (Module) Basic rm Utility using POSIX API and consio
// Codifiers: @Antigravity
// Attribute: Arn-Covenant Any-Architect Env-Hostedstanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <c/consio.h>
#include <c/ustring.h>

static int remove_recursive(const char* path) {
	struct stat st;
	if (stat(path, &st) < 0) {
		// If stat fails, attempt to delete directly via unlink
		return unlink(path);
	}

	if (S_ISDIR(st.st_mode)) {
		DIR* dir = opendir(path);
		if (!dir) {
			return -1;
		}

		struct dirent* entry;
		int has_error = 0;
		while ((entry = readdir(dir)) != NULL) {
			// Skip "." and ".." self/parent references
			if (StrCompare(entry->d_name, ".") == 0 || StrCompare(entry->d_name, "..") == 0) {
				continue;
			}

			// Construct child path safely
			char fullpath[512];
			stduint len = 0;
			while (path[len] && len < 500) {
				fullpath[len] = path[len];
				len++;
			}
			if (len > 0 && fullpath[len - 1] != '/') {
				fullpath[len++] = '/';
			}
			stduint name_len = 0;
			while (entry->d_name[name_len] && len < 511) {
				fullpath[len++] = entry->d_name[name_len++];
			}
			fullpath[len] = '\0';

			// Recursively delete children
			if (remove_recursive(fullpath) < 0) {
				has_error = 1;
			}
		}
		closedir(dir);
		if (has_error) {
			return -1;
		}
	}

	// Remove the file or the empty directory
	return unlink(path);
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		outsfmt("rm: missing operand\n\r");
		return 1;
	}

	int opt_recursive = 0;
	int start_idx = 1;

	// Parse command line arguments
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			for (int j = 1; argv[i][j] != '\0'; j++) {
				if (argv[i][j] == 'r' || argv[i][j] == 'R') {
					opt_recursive = 1;
				} else {
					outsfmt("rm: invalid option -- '%c'\n\r", argv[i][j]);
					return 1;
				}
			}
			if (start_idx == i) {
				start_idx = i + 1;
			}
		}
	}

	if (start_idx >= argc) {
		outsfmt("rm: missing operand\n\r");
		return 1;
	}

	int has_error = 0;
	for (int i = start_idx; i < argc; i++) {
		const char* path = argv[i];
		struct stat st;
		if (stat(path, &st) < 0) {
			outsfmt("rm: cannot remove '%s': No such file or directory\n\r", path);
			has_error = 1;
			continue;
		}

		if (S_ISDIR(st.st_mode) && !opt_recursive) {
			outsfmt("rm: cannot remove '%s': Is a directory\n\r", path);
			has_error = 1;
			continue;
		}

		if (remove_recursive(path) < 0) {
			outsfmt("rm: cannot remove '%s'\n\r", path);
			has_error = 1;
		}
	}

	return has_error ? 1 : 0;
}
