// ASCII C/C++ TAB4 CRLF
// Docutitle: (Format: File) Executable and Linkable Format
// Codifiers: @dosconio: 20240422 ~ <Last-check> 
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

#ifndef _INC_ELF
#define _INC_ELF

#include "../stdinc.h"

enum Bitmode_t {
	ELF32 = 1,
	ELF64 = 2,
};
enum Endianness_t {
	ELFEndianLittle = 1, // 2's complement
	ELFEndianBig = 2, // 2's complement
};
enum ELFVersion_t {
	ELFCurrentVersion = 1,
};
enum OSABI_t { //{TODO} use as public enum
	OSABI_UNIX_System_V = 0,
	OSABI_Hewlett_Packard_HP_UX = 1,
	OSABI_NetBSD = 2,
	OSABI_GNU_Hurd = 3,
	OSABI_Linux = 3,
	OSABI_Sun_Solaris = 6,
	OSABI_AIX = 7,
	OSABI_IRIX = 8,
	OSABI_FreeBSD = 9,
	OSABI_Compaq_Tru64_UNIX = 0x0A,
	OSABI_Novell_Modesto = 0x0B,
	OSABI_OpenBSD = 0x0C,
	OSABI_OpenVMS = 0x0D,
	OSABI_Hewlett_Packard_Non_Stop_Kernel = 0x0E,
	OSABI_Amiga_Research = 0x0F,
	OSABI_ARM_EABI = 0x40,
	OSABI_ARM = 0x61,
	//
	OSABI_Standalone_Application = 0xFF,
};
enum e_type_t {
	ET_NONE = 0,
	ET_REL = 1,
	ET_EXEC = 2,
	ET_DYN = 3,
	ET_CORE = 4,
	//
	ET_LOOS = 0xFE00,
	ET_HIOS = 0xFEFF,
	ET_LOPROC = 0xFF00,
	ET_HIPROC = 0xFFFF,
};
enum e_machine_t { //{ISSUE} same with UNISYM's proctrl&architecture?
	MAC_NONE //{TODO}
};
//
enum p_type_t {
	PT_NULL = 0,
	PT_LOAD = 1,
	PT_DYNAMIC = 2,
	PT_INTERP = 3,
	PT_NOTE = 4,
	PT_SHLIB = 5,
	PT_PHDR = 6,
	PT_TLS = 7,
	PT_LOOS = 0x60000000,
	PT_GNU_EH_FRAME = 0x6474e550,
	PT_GNU_STACK = 0x6474e551,
	PT_GNU_RELRO = 0x6474e552,
	PT_LOSUNW = 0x6ffffffa,
	PT_SUNWBSS = 0x6ffffffc,
	PT_SUNWSTACK = 0x6ffffffb,
	PT_HISUNW = 0x6fffffff,
	PT_HIOS = 0x6ffffffe,
	PT_LOPROC = 0x70000000,
	PT_HIPROC = 0x7fffffff,
	PT_ARM_EXIDX = 0x70000001,
	PT_ARM_PREEMPTMAP = 0x70000002,
	PT_ARM_ATTRIBUTES = 0x70000003,
	PT_ARM_DEBUGOVERLAY = 0x70000004,
	PT_ARM_OVERLAYSECTION = 0x70000005,
};



#if __BITS__ == 32
	#define EI_NIDENT 16
	typedef uint32 Elf32_Addr ;
	typedef uint16 Elf32_Half ;
	typedef uint32 Elf32_Off  ;
	typedef sint32 Elf32_SWord;
	typedef uint32 Elf32_Word ;
	//
	struct ELF_Header_t {
		union {
			byte  e_ident[EI_NIDENT];
			struct {
				byte MagicNumber[4];
				byte Bitmode;
				byte Endianness;
				byte ELFVersion;
				byte OSABI;
				byte ABIVersion;
				byte Pads[7];
			};
		};
		Elf32_Half e_type     ;
		Elf32_Half e_machine  ;
		Elf32_Word e_version  ;
		Elf32_Addr e_entry    ; // entry address
		Elf32_Off  e_phoff    ; // offset of program header table
		Elf32_Off  e_shoff    ; // offset of section header table
		Elf32_Word e_flags    ;
		Elf32_Half e_ehsize   ; // size of ELF header
		Elf32_Half e_phentsize; // size of program header table entry
		Elf32_Half e_phnum    ; // number of program header table entries
		Elf32_Half e_shentsize; // size of section header table entry
		Elf32_Half e_shnum    ; // number of section header table entries
		Elf32_Half e_shstrndx ; // section header table index of section name string table
	};
	struct ELF_PHT_t {
		Elf32_Word p_type  ; //{TODO}
		Elf32_Off  p_offset; // offset of segment in file
		Elf32_Addr p_vaddr ; // virtual address of segment in memory
		Elf32_Addr p_paddr ; // physical address of segment in memory
		Elf32_Word p_filesz; // size of segment in file
		Elf32_Word p_memsz ; // size of segment in memory
		Elf32_Word p_flags ; // [?READ ?WRITE ?EXEC]
		Elf32_Word p_align ; // the alignment of segment in memory
	};
	struct ELF_SHT_t {
		Elf32_Word sh_name     ;
		Elf32_Word sh_type     ;
		Elf32_Word sh_flags    ;
		Elf32_Addr sh_addr     ;
		Elf32_Off  sh_offset   ;
		Elf32_Word sh_size     ;
		Elf32_Word sh_link     ;
		Elf32_Word sh_info     ;
		Elf32_Word sh_addralign;
		Elf32_Word sh_entsize  ;
	};
	// PHT [e_phoff ~ e_phoff + e_phnum*e_phentsize *] (suf-* means not including)
#elif __BITS__ == 64
	//
#else
	//
#endif

extern unsigned _Literal_ELFBitmode[];

extern const char* _Literal_OSABI[];

extern const char* _Literal_ELFType[];

extern const char* _Literal_ELFMachine[];

extern const char* getLiteral_ELFProgramHeaderType(uint32 typ);

extern const char* _Literal_ELFProgramFlags[];

extern const char* getLiteral_ELFSectionHeaderType(uint32 typ);

extern const char* getLiteral_ELFSectionFlags(byte typ);

//

// ELF32_LoadFromMemory //{TODO} ELF<32>::LoadFromMemory
//{TEMP} no check for validation
// return how many PHBlock has been loaded
stduint ELF32_LoadExecFromMemory(const void* memsrc, void** p_entry);

#endif
