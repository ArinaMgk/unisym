#include <unistd.h>
#include <fcntl.h>
#include <c/consio.h>

#define PAGELEN 24
#define BUFSIZE 512

static int prompt_more() {
	outs("\n\r--more--");
	char c = 0;
	stdsint n = read(0, &c, 1);
	outs("\n\r");
	if (n <= 0) return 0;
	if (c == 'q' || c == 'Q') return 0;
	if (c == ' ') return PAGELEN;
	if (c == '\n' || c == '\r') return 1;
	return PAGELEN;
}

static int do_more_fd(int fd) {
	char buf[BUFSIZE];
	int lines_left = PAGELEN;
	while (true) {
		stdsint nr = read(fd, buf, sizeof(buf));
		if (nr < 0) {
			return 1;
		}
		if (nr == 0) {
			break;
		}
		for (stdint i = 0; i < nr; i++) {
			char ch = buf[i];
			if (write(1, &ch, 1) < 0) {
				return 1;
			}
			if (ch == '\n') {
				lines_left--;
				if (lines_left <= 0) {
					int reply = prompt_more();
					if (reply == 0) {
						return 0;
					}
					lines_left = reply;
				}
			}
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		return do_more_fd(0);
	}

	int ret = 0;
	for (int i = 1; i < argc; i++) {
		int fd = open(argv[i], O_RDONLY);
		if (fd < 0) {
			outsfmt("more: %s: cannot open\n\r", argv[i]);
			ret = 1;
			continue;
		}
		int one = do_more_fd(fd);
		close(fd);
		if (one != 0) {
			ret = one;
		}
	}
	return ret;
}