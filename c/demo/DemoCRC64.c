// RFC07 ASCII TAB4 C99 ArnAssume
// <CRC64 compatible with 7-Zip>
// Build: gcc DemoCRC64.c ../source/crc64.c -I../
#include "../crc64.h"
#include <stdio.h>
#include <stdlib.h>

FILE* file;
uint64_t crc;
int main() {
	if ((file = fopen(__FILE__, "rb")) == NULL)
		return 1;
	#if 0
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	unsigned char* buffer = malloc(file_size);
	if (buffer == NULL)
	{
		fclose(file);
		return 1;
	}
	if (fread(buffer, file_size, 1, file) != 1)
	{
		free(buffer);
		fclose(file);
		return 1;
	}
	fclose(file);
	crc = CRC64(buffer, file_size, CRC64_ECMA_Initial, CRC64_ECMA_Polynomial, CRC64_ECMA_finalxor, -1);
	printf("CRC64 of " __FILE__ ": %016llX\n", crc);
	free(buffer);
	#else// ---- ----

	crc = CRC64_ECMA_Initial;
	int chr = 0;
	while ((chr = fgetc(file)) != EOF)
		crc = CRC64Once(crc, chr, CRC64_ECMA_Polynomial, -1);
	crc = CRC64Endo(crc, CRC64_ECMA_finalxor, -1);
	printf("CRC64 of " __FILE__ ": %016llX\n", crc);
	fclose(file);
	#endif
	return 0;
}


