#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <c/consio.h>

#define SORT_BUFSIZE 1024
#define SORT_LINEMAX 4096

static stduint str_length(const char* s) {
	stduint n = 0;
	while (s[n]) n++;
	return n;
}

static int str_compare(const char* a, const char* b) {
	for (stduint i = 0; ; i++) {
		unsigned char ca = (unsigned char)a[i];
		unsigned char cb = (unsigned char)b[i];
		if (ca != cb) return (ca < cb) ? -1 : 1;
		if (!ca) return 0;
	}
}

static char* str_dup_n(const char* s, stduint len) {
	char* p = (char*)malloc(len + 1);
	if (!p) return nullptr;
	for (stduint i = 0; i < len; i++) p[i] = s[i];
	p[len] = '\0';
	return p;
}

static int read_lines_from_fd(int fd, char*** out_lines, stduint* out_count) {
	char** lines = nullptr;
	stduint count = 0;
	stduint cap = 0;
	char buf[SORT_BUFSIZE];
	char line[SORT_LINEMAX];
	stduint line_len = 0;
	while (true) {
		stdsint nr = read(fd, buf, sizeof(buf));
		if (nr < 0) {
			return 1;
		}
		if (nr == 0) break;
		for (stdint i = 0; i < nr; i++) {
			char ch = buf[i];
			if (line_len + 1 < SORT_LINEMAX) {
				line[line_len++] = ch;
			}
			if (ch == '\n') {
				char* copy = str_dup_n(line, line_len);
				if (!copy) return 1;
				if (count == cap) {
					stduint new_cap = cap ? (cap << 1) : 32;
					char** new_lines = (char**)realloc(lines, new_cap * sizeof(char*));
					if (!new_lines) return 1;
					lines = new_lines;
					cap = new_cap;
				}
				lines[count++] = copy;
				line_len = 0;
			}
		}
	}
	if (line_len > 0) {
		char* copy = str_dup_n(line, line_len);
		if (!copy) return 1;
		if (count == cap) {
			stduint new_cap = cap ? (cap << 1) : 32;
			char** new_lines = (char**)realloc(lines, new_cap * sizeof(char*));
			if (!new_lines) return 1;
			lines = new_lines;
			cap = new_cap;
		}
		lines[count++] = copy;
	}
	*out_lines = lines;
	*out_count = count;
	return 0;
}

static void insertion_sort(char** lines, stduint count) {
	for (stduint i = 1; i < count; i++) {
		char* key = lines[i];
		stdint j = (stdint)i - 1;
		while (j >= 0 && str_compare(lines[j], key) > 0) {
			lines[j + 1] = lines[j];
			j--;
		}
		lines[j + 1] = key;
	}
}

static void free_lines(char** lines, stduint count) {
	for (stduint i = 0; i < count; i++) free(lines[i]);
	free(lines);
}

int main(int argc, char* argv[]) {
	int fd = 0;
	if (argc >= 2) {
		fd = open(argv[1], O_RDONLY);
		if (fd < 0) {
			outsfmt("sort: %s: cannot open\n\r", argv[1]);
			return 1;
		}
	}

	char** lines = nullptr;
	stduint count = 0;
	if (read_lines_from_fd(fd, &lines, &count) != 0) {
		if (fd != 0) close(fd);
		outs("sort: read error\n\r");
		return 1;
	}
	if (fd != 0) close(fd);

	insertion_sort(lines, count);
	for (stduint i = 0; i < count; i++) {
		write(1, lines[i], str_length(lines[i]));
	}
	free_lines(lines, count);
	return 0;
}
/*
#include <cpp/unisym>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

char line[1024];
int main(int argc, char* argv[]) {

	FILE* fp = stdin;
	if (argc >= 2) {
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			fprintf(stderr, "Cannot open file %s\n", argv[1]);
			exit(1);
		}
	}

	std::vector<std::string> lines;
	while (fgets(line, sizeof(line), fp)) {
		lines.push_back(line);
	}

	auto comp = [](const std::string& a, const std::string& b) {
		for0(i, minof(a.length(), b.length())) {
			if (a[i] != b[i]) {
				return a[i] < b[i];
			}
		}
		return a.length() < b.length();
	};

	std::sort(lines.begin(), lines.end(), comp);
	for (auto& line : lines) {
		printf("%s", line.c_str());
	}
}


*/
