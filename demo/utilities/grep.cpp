#include <unistd.h>
#include <fcntl.h>
#include <c/consio.h>

#define GREP_BUFSIZE 1024
#define GREP_LINEMAX 4096

static stduint str_length(const char* s) {
	stduint n = 0;
	while (s[n]) n++;
	return n;
}

static bool contains_substr(const char* text, const char* pattern) {
	if (!pattern[0]) return true;
	for (stduint i = 0; text[i]; i++) {
		stduint j = 0;
		while (pattern[j] && text[i + j] && text[i + j] == pattern[j]) {
			j++;
		}
		if (!pattern[j]) return true;
	}
	return false;
}

static int grep_fd(int fd, const char* pattern) {
	char buf[GREP_BUFSIZE];
	char line[GREP_LINEMAX];
	stduint line_len = 0;
	while (true) {
		stdsint nr = read(fd, buf, sizeof(buf));
		if (nr < 0) {
			return 1;
		}
		if (nr == 0) break;
		for (stdint i = 0; i < nr; i++) {
			char ch = buf[i];
			if (line_len + 1 < GREP_LINEMAX) {
				line[line_len++] = ch;
			}
			if (ch == '\n') {
				line[line_len] = '\0';
				if (contains_substr(line, pattern)) {
					write(1, line, line_len);
				}
				line_len = 0;
			}
		}
	}
	if (line_len > 0) {
		line[line_len] = '\0';
		if (contains_substr(line, pattern)) {
			write(1, line, line_len);
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		outs("Usage: grep <pattern> [file ...]\n\r");
		return 1;
	}

	const char* pattern = argv[1];
	if (argc == 2) {
		return grep_fd(0, pattern);
	}

	int ret = 0;
	for (int i = 2; i < argc; i++) {
		int fd = open(argv[i], O_RDONLY);
		if (fd < 0) {
			outsfmt("grep: %s: cannot open\n\r", argv[i]);
			ret = 1;
			continue;
		}
		if (grep_fd(fd, pattern) != 0) {
			outsfmt("grep: %s: read error\n\r", argv[i]);
			ret = 1;
		}
		close(fd);
	}
	return ret;
}

/*
#include <cstdio>
#include <cstdlib>
#include <regex>

char line[256];
int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Usage: grep <pattern> <file>\n");
		return 1;
	}
	std::regex pattern(argv[1]);

	FILE* file = fopen(argv[2], "r");
	if (!file) {
		printf("Error: Cannot open file %s\n", argv[2]);
		return 1;
	}
	while (fgets(line, sizeof(line), file)) {
		std::cmatch m;
		if (std::regex_search(line, m, pattern)) {
			printf("%s", line);
		}
	}

}

*/
