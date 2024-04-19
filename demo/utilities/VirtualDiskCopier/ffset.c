
#include <stdio.h>
#include <inttypes.h>
#include "host.h"
#include "ustring.h"

// from branch source of COTLAB
// set part of file from a file.

int FFSet(const char* dest, const char* sors, size_t sector)// wo unisym/Kasha/kit/vdkcpy.cpp
{
	FILE* fp, * destfp;
	size_t bytes = 0;
	if ((fp = fopen(sors, "rb")) == nil)
	{
		fprintf(stderr, "Error: file not found\n");
		return -1;
	}
	if ((destfp = fopen(dest, "rb+")) == nil)
	{
		fprintf(stderr, "Error: file not found\n");
		return -1;
	}
	fseek(destfp, sector * 512, SEEK_SET);// 512 is the size of a sector
	int ch;
	while ((ch = fgetc(fp)) != EOF)
	{
		fputc(ch, destfp);
		bytes++;
	}
	printf("%s> %" PRIdPTR " bytes from source.\n", sors, bytes);
	fclose(fp);
	fclose(destfp);
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		fprintf(stderr, "Usage: ffset [dest_file] [sors_file] [sector_id]\n");
		return -1;
	}
	return FFSet(argv[1], argv[2], atoins(argv[3]));
}
