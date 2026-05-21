// ASCII C++ TAB4 CRLF
// Docutitle: (Module) Basic ls Utility using POSIX API and consio
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Hostedstanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "dirent.h"
#include <sys/stat.h>
#include <dirent.h>
#include <c/consio.h>

// Format file size into human-readable or raw bytes representation using integer math
static void format_size(char *buf, stduint size, bool human) {
	if (!human) {
		outsfmtbuf(buf, "%lu", (unsigned long)size);
		return;
	}
	if (size < 1024) {
		outsfmtbuf(buf, "%luB", (unsigned long)size);
	} else if (size < 1024 * 1024) {
		unsigned long kb = size / 1024;
		unsigned long frac = ((size % 1024) * 10) / 1024;
		outsfmtbuf(buf, "%lu.%luK", kb, frac);
	} else if (size < 1024 * 1024 * 1024) {
		unsigned long mb = size / (1024 * 1024);
		unsigned long frac = (((size % (1024 * 1024)) * 10) / (1024 * 1024));
		outsfmtbuf(buf, "%lu.%luM", mb, frac);
	} else {
		unsigned long gb = size / (1024 * 1024 * 1024);
		unsigned long frac = (((size % (1024 * 1024 * 1024)) * 10) / (1024 * 1024 * 1024));
		outsfmtbuf(buf, "%lu.%luG", gb, frac);
	}
}

int main(int argc, char *argv[]) {
	bool opt_l = false;
	bool opt_h = false;
	const char* path = ".";

	// Parse command line arguments
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			for (int j = 1; argv[i][j] != '\0'; j++) {
				if (argv[i][j] == 'l') {
					opt_l = true;
				} else if (argv[i][j] == 'h') {
					opt_h = true;
				} else {
					outsfmt("ls: invalid option -- '%c'\n\r", argv[i][j]);
					return 1;
				}
			}
		} else {
			path = argv[i];
		}
	}

	// Try to open the target path as a directory
	DIR* dir = opendir(path);
	if (!dir) {
		// If opendir fails, check if path points to a regular file
		struct stat st;
		if (stat(path, &st) == 0) {
			if (opt_l) {
				char szbuf[32];
				format_size(szbuf, st.st_size, opt_h);
				outsfmt("-rw-r--r--  %s  %s\n\r", szbuf, path);
			} else {
				outsfmt("%s\n\r", path);
			}
			return 0;
		}
		outsfmt("ls: cannot access '%s': No such file or directory\n\r", path);
		return 1;
	}

	// Enumerate directory entries
	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr) {
		if (opt_l) {
			char fullpath[512];
			stduint path_len = 0;
			while (path[path_len]) {
				fullpath[path_len] = path[path_len];
				path_len++;
			}
			if (path_len > 0 && fullpath[path_len - 1] != '/') {
				fullpath[path_len++] = '/';
			}
			
			stduint entry_name_len = 0;
			while (entry->d_name[entry_name_len]) {
				fullpath[path_len++] = entry->d_name[entry_name_len++];
			}
			fullpath[path_len] = '\0';

			struct stat st;
			if (stat(fullpath, &st) == 0) {
				char szbuf[32];
				format_size(szbuf, st.st_size, opt_h);
				const char* type_str = S_ISDIR(st.st_mode) ? "drwxr-xr-x" : "-rw-r--r--";
				outsfmt("%s  %s  %s\n\r", type_str, szbuf, entry->d_name);
			} else {
				const char* type_str = (entry->d_type == DT_DIR) ? "drwxr-xr-x" : "-rw-r--r--";
				outsfmt("%s  ?  %s\n\r", type_str, entry->d_name);
			}
		} else {
			outsfmt("%s  ", entry->d_name);
		}
	}
	if (!opt_l) {
		outsfmt("\n\r");
	}

	closedir(dir);
	return 0;
}
