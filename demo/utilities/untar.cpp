
#include "../../inc/cpp/unisym"
//
#include <stdio.h>
#include "../../inc/c/file.h"
#include "../../inc/c/consio.h"
#include "../../inc/c/format/data/tar.h"


using namespace uni;

#define SECTOR_SIZE 512
byte buf[SECTOR_SIZE];

void untar(rostr filename, rostr out_folder)
{
	HostFile tarf(filename, FileOpenType::Read);
	String fullpath = out_folder;
	if (fullpath[-1] != '/') 
		fullpath += "/";
	while (true) {
		for0(i, SECTOR_SIZE) {
			if (tarf >> buf[i]); else return;
		}
		if (!buf[0]) break;
		Letvar(phdr, data_tar_header*, buf);
		// calculate the file size
		char* p = phdr->size;
		int f_len = 0;
		while (*p) f_len = (f_len * 8) + (*p++ - '0'); // octal
		//
		String outf_path = fullpath + phdr->name;
		Console.OutFormat("%s # length 0x%x\n", outf_path.reference(), f_len);
		HostFile outf(outf_path.reference(), FileOpenType::Write);
		for0(i, f_len) {
			byte ch;
			tarf >> ch;// assert
			outf << ch;
		}
		stduint last = f_len % SECTOR_SIZE;
		if (last) for0(i, SECTOR_SIZE - last) tarf.inn();
	}
}

int main(int argc, char ** argv) {
	if (argc == 3) {
		untar(argv[1], argv[2]);
	}
	else return -1;
	return malc_count;
}
