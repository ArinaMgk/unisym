// ASCII C/C++ TAB4 CRLF
// Docutitle: (Format: File) Executable and Linkable Format
// Codifiers: @dosconio: 20240427 ~ <Last-check> 
// Attribute: Arn-Covenant Any-Architect <Environment> <Reference/Dependence>
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../../inc/c/format/ELF.h"
#include "../../inc/c/ustring.h"
#include "stdlib.h"
#include "stdio.h"

//{TEMP} just test in 32-bitmode, 2'little-endian

typedef const char* strtmpdef[];
//{TEMP} no boundary check

static void showelf(const char* elftext)
{
	struct ELF_Header_t* header = (struct ELF_Header_t*)elftext;
	// for0(i, numsof(header->MagicNumber)) printf("%c", header->MagicNumber[i]);
	if (StrCompareN(header->MagicNumber, "\x7F" "ELF", 4))
	{
		puts("\tNot an ELF file");
		return;
	}
	puts("---- ELF Header ----");
	printf("Size(%u)\n", header->e_ehsize);
	printf("\tIs an ELF%u file\n", _Literal_ELFBitmode[header->Bitmode]);
	printf("Endian\t%s\n", ((strtmpdef){"2'complement Little", "2'complement Big"})[header->Endianness]);
	printf("Version\t%u Target 0x%x\n", header->ELFVersion, header->e_version);//{TODO}
	printf("OS/ABI\t%s, ABI Version%u\n", _Literal_OSABI[header->OSABI], header->ABIVersion);
	printf("Type\t%s\n", _Literal_ELFType[header->e_type]);
	printf("Machine\t%s\n", _Literal_ELFMachine[header->e_machine]);
	printf("Entry\t0x%08x\n", header->e_entry);
	if (0)
	{
		printf("PH-Off\t0x%08x\n", header->e_phoff);
		printf("SH-Off\t0x%08x\n", header->e_shoff);
		printf("Flags\t0x%08x\n", header->e_flags);
		printf("HeadSiz\t%u\n", header->e_ehsize);
		printf("PHE-Siz\t%u\n", header->e_phentsize);
		printf("PHE-Num\t%u\n", header->e_phnum);
		printf("SHE-Siz\t%u\n", header->e_shentsize);
		printf("SHE-Num\t%u\n", header->e_shnum);
		printf("Str-Idx\t%u\n", header->e_shstrndx);
	}

	puts("---- Program Header Table ----");
	printf("Offset(0x%08x) Unit(%u) Count(%u) \n", header->e_phoff, header->e_phentsize, header->e_phnum);
	printf("Type\t\tOffset\tVAddres\tPAddres\tFilSize\tMemSize\tFlag&Align\n");
	for0 (i, header->e_phnum)
	{
		struct ELF_PHT_t* ph = (struct ELF_PHT_t*)(elftext + header->e_phoff + header->e_phentsize * i);
		printf("%-15s\t%06xH\t%06xH\t%06xH\t%06xH\t%06xH\t%-4s-%04xH\n", getLiteral_ELFProgramHeaderType(ph->p_type), ph->p_offset, ph->p_vaddr, ph->p_paddr, ph->p_filesz, ph->p_memsz, _Literal_ELFProgramFlags[ph->p_flags], ph->p_align);
	}


	puts("---- Section Header Table ----");
	printf("Offset(0x%08x) Unit(%u) Count(%u) \n", header->e_shoff, header->e_shentsize, header->e_shnum);
	struct ELF_SHT_t* strsh = (struct ELF_SHT_t*)(elftext + header->e_shoff + header->e_shentsize * header->e_shstrndx);
	printf("No. %-15s %-15s [Address]&[Offset] Size    ES Flg Lk Inf Al\n", "Name", "Type");
	for0 (i, header->e_shnum)
	{
		struct ELF_SHT_t* sh = (struct ELF_SHT_t*)(elftext + header->e_shoff + header->e_shentsize * i);
		printf("%3u %-15s %-15s %08xH %07xH %06xH %2u %3s %2u %3u %2u\n", i,
			elftext + strsh->sh_offset + sh->sh_name,
			getLiteral_ELFSectionHeaderType(sh->sh_type),
			sh->sh_addr, sh->sh_offset, sh->sh_size, sh->sh_entsize,
			getLiteral_ELFSectionFlags(sh->sh_flags), 
			sh->sh_link, sh->sh_info, sh->sh_addralign);
	}


	// ---- ELF ? ----
}

static void readelf(FILE* fp)
{
	long file_size;
	long file_crtoff = ftell(fp);
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, file_crtoff, SEEK_SET);
	char* buf = malc(file_size);
	fread(buf, 1, file_size, fp);
	showelf(buf);
	mfree(buf);
}

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage: %s <ELF-file>\n", argv[0]);
		return 0;
	}
	FILE* fp = fopen(argv[1], "rb");
	if(fp == NULL)
	{
		fprintf(stderr, "Can't open file: %s\n", argv[1]);
		return 0;
	}
	readelf(fp);
	fclose(fp);
}


