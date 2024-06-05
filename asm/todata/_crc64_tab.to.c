// ASCII C99 TAB4 CRLF
// Attribute: /
// LastCheck: 2024Jan26
// AllAuthor: @dosconio
// ModuTitle: Make C Source File for CRC64 Static Table like NASM0207

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#define POLY64REV 0x95AC9329AC4BC9B5ULL; // NASM 2.7 uses this

void init_crc64_tab(uint64_t* crc64_tab)
{
	
	for (int i = 0; i < 256; i++) {
		uint64_t crc = i;
		for (int j = 0; j < 8; j++) {
			if (crc & 1) {
				crc = (crc >> 1) ^ POLY64REV;
			} else {
				crc >>= 1;
			}
		}
		crc64_tab[i] = crc;
	}
}

int main()
{
	uint64_t crc64_tab[256] = { 0 };
	FILE* fptr = NULL;
	init_crc64_tab(&crc64_tab[0]);// also passing `[0]` to avoid warning `expected 'uint64_t *'`
	fopen_s(&fptr, "_crc64_tab.c", "w");
	//
	fprintf(fptr, "// ASCII C99 TAB4 CRLF\n");
	fprintf(fptr, "// Attribute: Wo(USYM/asm/data/_crc64_tab.to.c)\n");
	fprintf(fptr, "// LastCheck: 2024Jan26\n");
	fprintf(fptr, "// CreateTim: " __DATE__ "\n");
	fprintf(fptr, "// AllAuthor: @dosconio\n");
	fprintf(fptr, "// ModuTitle: CRC64 Static Table like NASM0207\n");
	fprintf(fptr, "\n");
	fprintf(fptr, "#include \"../../inc/c/stdinc.h\"\n");
	fprintf(fptr, "#include <stdio.h>\n");
	fprintf(fptr, "\n");
	fprintf(fptr, "uint64_t _crc64_tab_nasm[256] =\n");
	fprintf(fptr, "{\n\t");
	for (int i = 0; i < 256; i++) {
		fprintf(fptr, "0x%016" PRIx64 "ULL, ", crc64_tab[i]);
		if (i % 4 == 3) {
			if (i != 255)
				fprintf(fptr, "\n\t");
			else
				fprintf(fptr, "\n");
		}
	}
	fprintf(fptr, "};\n");
	//
	fclose(fptr);

	return 0;
}