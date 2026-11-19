// ASCII C TAB4 CRLF
// Docutitle: (Module) Basic mv Utility using POSIX API and consio
// Attribute: Arn-Covenant Any-Architect Env-Hostedstanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <c/consio.h>
#include <c/ustring.h>
#include <c/ISO_IEC_STD/errno.h>

int main(int argc, char* argv[]) {
	if (argc != 3) {
		outsfmt("Usage: mv SOURCE DESTINATION\n\r");
		return 1;
	}
	const char* source = argv[1];
	const char* destination = argv[2];
	const stduint capacity = 512;
	char* target = (char*)malloc(capacity);
	if (!target) {
		outsfmt("mv: out of memory\n\r");
		return 1;
	}
	stduint length = StrLength(destination);
	if (length >= capacity) {
		outsfmt("mv: destination path too long\n\r");
		free(target);
		return 1;
	}
	MemCopyN(target, destination, length + 1);
	struct stat info;
	const int destination_status = stat(destination, &info);
	if (destination_status < 0 && errno != ENOENT) {
		outsfmt("mv: cannot access '%s' (errno=%d)\n\r", destination, errno);
		free(target);
		return 1;
	}
	// An existing destination directory receives the source basename, ignoring source trailing slashes.
	if (destination_status == 0 && S_ISDIR(info.st_mode)) {
		const char* end = source + StrLength(source);
		while (end > source && end[-1] == '/') --end;
		const char* name = end;
		while (name > source && name[-1] != '/') --name;
		const stduint name_length = end - name;
		const stduint separator = length && target[length - 1] != '/' ? 1 : 0;
		if (!name_length || name_length > capacity - length - 1 || separator > capacity - length - 1 - name_length) {
			outsfmt("mv: invalid or too long destination path\n\r");
			free(target);
			return 1;
		}
		if (separator) target[length++] = '/';
		MemCopyN(target + length, name, name_length);
		target[length + name_length] = '\0';
	}
	// Keep rename failures intact instead of modifying files through a copy-and-delete fallback.
	const int result = rename(source, target);
	if (result < 0) outsfmt("mv: cannot move '%s' to '%s' (errno=%d)\n\r", source, target, errno);
	free(target);
	return result < 0 ? 1 : 0;
}
