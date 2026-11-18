// ASCII C++ TAB4 CRLF
// Docutitle: (Module) Basic ls Utility using POSIX API and consio
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Hostedstanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "dirent.h"
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <c/consio.h>

// POSIX ls long format uses the file type, rwx bits, and set-ID/sticky substitutions from st_mode.
static void format_mode(char* buf, stduint mode) {
	switch (mode & S_IFMT) {
	case S_IFREG: buf[0] = '-'; break;
	case S_IFDIR: buf[0] = 'd'; break;
	case S_IFCHR: buf[0] = 'c'; break;
	#ifdef S_IFBLK
	case S_IFBLK: buf[0] = 'b'; break;
	#endif
	#ifdef S_IFIFO
	case S_IFIFO: buf[0] = 'p'; break;
	#endif
	#ifdef S_IFLNK
	case S_IFLNK: buf[0] = 'l'; break;
	#endif
	#ifdef S_IFSOCK
	case S_IFSOCK: buf[0] = 's'; break;
	#endif
	default: buf[0] = '?'; break;
	}
	const char* bits = "rwxrwxrwx";
	for (unsigned i = 0; i < 9; i++) buf[i + 1] = (mode & (0400u >> i)) ? bits[i] : '-';
	#ifdef S_ISUID
	if (mode & S_ISUID) buf[3] = buf[3] == 'x' ? 's' : 'S';
	#endif
	#ifdef S_ISGID
	if (mode & S_ISGID) buf[6] = buf[6] == 'x' ? 's' : 'S';
	#endif
	#ifdef S_ISVTX
	if (mode & S_ISVTX) buf[9] = buf[9] == 'x' ? 't' : 'T';
	#endif
	buf[10] = '\0';
}

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

static void format_time(char* buf, const struct stat& st) {
	if (st.st_mtime) {
		const struct tm* date = localtime(&st.st_mtime);
		if (date && strftime(buf, 32, "%Y-%m-%d %H:%M", date)) return;
	}
	buf[0] = '-';
	buf[1] = '\0';
}

// Share the same metadata formatting for individual paths and enumerated directory entries.
static void print_long_entry(const char* name, const struct stat& st, bool human, char* identity_buffer) {
	char modebuf[11];
	char szbuf[32];
	char timebuf[32];
	format_mode(modebuf, st.st_mode);
	format_size(szbuf, st.st_size, human);
	format_time(timebuf, st);
	char owner_number[24];
	char group_number[24];
	outsfmtbuf(owner_number, "%ld", (long)st.st_uid);
	outsfmtbuf(group_number, "%ld", (long)st.st_gid);
	const char* owner = owner_number;
	const char* group_name = group_number;
	if (identity_buffer) {
		// Keep both query buffers alive until the row is printed; unknown IDs and lookup errors retain numbers.
		struct passwd user{};
		struct passwd* found_user = nullptr;
		if (!getpwuid_r(st.st_uid, &user, identity_buffer, 256, &found_user) && found_user) owner = user.pw_name;
		struct group group{};
		struct group* found_group = nullptr;
		if (!getgrgid_r(st.st_gid, &group, identity_buffer + 256, 256, &found_group) && found_group) group_name = group.gr_name;
	}
	outsfmt("%s  %s  %s  %s  %s  %s\n\r", modebuf, owner, group_name, szbuf, timebuf, name);
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

	// Reuse heap storage across rows instead of adding account buffers to the stack.
	char* identity_buffer = opt_l ? (char*)malloc(512) : nullptr;

	// Try to open the target path as a directory
	DIR* dir = opendir(path);
	if (!dir) {
		// If opendir fails, check if path points to a regular file
		struct stat st;
		if (stat(path, &st) == 0) {
			if (opt_l) {
				print_long_entry(path, st, opt_h, identity_buffer);
			} else {
				outsfmt("%s\n\r", path);
			}
			free(identity_buffer);
			return 0;
		}
		free(identity_buffer);
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
				print_long_entry(entry->d_name, st, opt_h, identity_buffer);
			} else {
				const char* type_str = (entry->d_type == DT_DIR) ? "d?????????" : "??????????";
				outsfmt("%s  ?  ?  ?  -  %s\n\r", type_str, entry->d_name);
			}
		} else {
			outsfmt("%s  ", entry->d_name);
		}
	}
	if (!opt_l) {
		outsfmt("\n\r");
	}

	closedir(dir);
	free(identity_buffer);
	return 0;
}
