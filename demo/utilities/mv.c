// ASCII C TAB4 CRLF
// Docutitle: (Module) Basic mv Utility using POSIX API and consio
// Codifiers: @Antigravity
// Attribute: Arn-Covenant Any-Architect Env-Hostedstanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <c/consio.h>
#include <c/ustring.h>

static const char* get_filename(const char* path) {
	const char* p = path;
	const char* last = path;
	while (*p) {
		if (*p == '/') {
			last = p + 1;
		}
		p++;
	}
	return last;
}

static int copy_file(const char* src, const char* dest) {
	int fd_in = open(src, O_RDONLY);
	if (fd_in < 0) {
		return -1;
	}

	int fd_out = open(dest, O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if (fd_out < 0) {
		close(fd_in);
		return -1;
	}

	char buf[1024];
	stdsint bytes_read;
	int has_error = 0;

	while ((bytes_read = read(fd_in, buf, sizeof(buf))) > 0) {
		stduint total_written = 0;
		while (total_written < (stduint)bytes_read) {
			stdsint bytes_written = write(fd_out, buf + total_written, bytes_read - total_written);
			if (bytes_written < 0) {
				has_error = 1;
				break;
			}
			total_written += bytes_written;
		}
		if (has_error) {
			break;
		}
	}

	if (bytes_read < 0) {
		has_error = 1;
	}

	close(fd_in);
	close(fd_out);

	return has_error ? -1 : 0;
}

static int copy_recursive(const char* src, const char* dest) {
	struct stat st;
	if (stat(src, &st) < 0) {
		return -1;
	}

	if (S_ISDIR(st.st_mode)) {
		mkdir(dest, 0777);

		DIR* dir = opendir(src);
		if (!dir) {
			return -1;
		}

		struct dirent* entry;
		int has_error = 0;
		while ((entry = readdir(dir)) != NULL) {
			if (StrCompare(entry->d_name, ".") == 0 || StrCompare(entry->d_name, "..") == 0) {
				continue;
			}

			char sub_src[512];
			stduint len_src = 0;
			while (src[len_src] && len_src < 500) {
				sub_src[len_src] = src[len_src];
				len_src++;
			}
			if (len_src > 0 && sub_src[len_src - 1] != '/') {
				sub_src[len_src++] = '/';
			}
			stduint name_len = 0;
			while (entry->d_name[name_len] && len_src < 511) {
				sub_src[len_src++] = entry->d_name[name_len++];
			}
			sub_src[len_src] = '\0';

			char sub_dest[512];
			stduint len_dest = 0;
			while (dest[len_dest] && len_dest < 500) {
				sub_dest[len_dest] = dest[len_dest];
				len_dest++;
			}
			if (len_dest > 0 && sub_dest[len_dest - 1] != '/') {
				sub_dest[len_dest++] = '/';
			}
			name_len = 0;
			while (entry->d_name[name_len] && len_dest < 511) {
				sub_dest[len_dest++] = entry->d_name[name_len++];
			}
			sub_dest[len_dest] = '\0';

			if (copy_recursive(sub_src, sub_dest) < 0) {
				has_error = 1;
			}
		}
		closedir(dir);
		return has_error ? -1 : 0;
	} else {
		return copy_file(src, dest);
	}
}

static int remove_recursive(const char* path) {
	struct stat st;
	if (stat(path, &st) < 0) {
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
			if (StrCompare(entry->d_name, ".") == 0 || StrCompare(entry->d_name, "..") == 0) {
				continue;
			}

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

			if (remove_recursive(fullpath) < 0) {
				has_error = 1;
			}
		}
		closedir(dir);
		if (has_error) {
			return -1;
		}
	}

	return unlink(path);
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		outsfmt("mv: missing file operand\n\r");
		return 1;
	}

	const char* src = argv[1];
	const char* dest = argv[2];

	struct stat st_src;
	if (stat(src, &st_src) < 0) {
		outsfmt("mv: cannot stat '%s': No such file or directory\n\r", src);
		return 1;
	}

	struct stat st_dest;
	char final_dest[512];
	stduint d_idx = 0;
	while (dest[d_idx] && d_idx < 511) {
		final_dest[d_idx] = dest[d_idx];
		d_idx++;
	}
	final_dest[d_idx] = '\0';

	// If destination is an existing directory, copy inside it
	if (stat(dest, &st_dest) == 0 && S_ISDIR(st_dest.st_mode)) {
		const char* fname = get_filename(src);
		if (d_idx > 0 && final_dest[d_idx - 1] != '/') {
			final_dest[d_idx++] = '/';
		}
		stduint f_len = 0;
		while (fname[f_len] && d_idx < 511) {
			final_dest[d_idx++] = fname[f_len++];
		}
		final_dest[d_idx] = '\0';
	}

	// Move is achieved by copying first, then recursively removing source
	if (copy_recursive(src, final_dest) < 0) {
		outsfmt("mv: failed to move '%s' to '%s'\n\r", src, final_dest);
		return 1;
	}

	if (remove_recursive(src) < 0) {
		outsfmt("mv: warning: failed to remove source '%s' after copy\n\r", src);
		return 1;
	}

	return 0;
}
