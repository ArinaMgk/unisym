// ASCII C++ TAB4 CRLF
// Docutitle: Process identity utility
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <c/consio.h>

int main(int argc, char* argv[]) {
	if (argc != 1) {
		outsfmt("Usage: id\n\r");
		return 1;
	}
	const uid_t uid = getuid();
	const gid_t gid = getgid();
	if (uid < 0 || gid < 0) return 1;
	const int count = getgroups(0, nullptr);
	if (count < 0 || size_t(count) > ~size_t(0) / sizeof(gid_t)) return 1;
	auto* groups = count ? static_cast<gid_t*>(malloc(size_t(count) * sizeof(gid_t))) : nullptr;
	if (count && !groups) return 1;
	const int received = count ? getgroups(count, groups) : 0;
	if (received < 0) { free(groups); return 1; }
	char* buffer = (char*)malloc(256);
	if (!buffer) { free(groups); return 1; }
	struct passwd user{};
	struct passwd* found_user = nullptr;
	const int user_status = getpwuid_r(uid, &user, buffer, 256, &found_user);
	outsfmt("uid=%lu", (unsigned long)uid);
	if (!user_status && found_user) outsfmt("(%s)", user.pw_name);
	struct group group{};
	struct group* found_group = nullptr;
	const int group_status = getgrgid_r(gid, &group, buffer, 256, &found_group);
	outsfmt(" gid=%lu", (unsigned long)gid);
	if (!group_status && found_group) outsfmt("(%s)", group.gr_name);
	outsfmt(" groups=%lu", (unsigned long)gid);
	if (!group_status && found_group) outsfmt("(%s)", group.gr_name);
	for (int i = 0; i < received; i++) {
		if (groups[i] == gid) continue;
		found_group = nullptr;
		const int status = getgrgid_r(groups[i], &group, buffer, 256, &found_group);
		outsfmt(",%lu", (unsigned long)groups[i]);
		if (!status && found_group) outsfmt("(%s)", group.gr_name);
	}
	outsfmt("\n\r");
	free(groups);
	free(buffer);
	return 0;
}
