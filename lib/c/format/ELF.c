// ASCII C/C++ TAB4 CRLF
// Docutitle: (Format: File) Executable and Linkable Format
// Codifiers: @dosconio: 20240427 ~ <Last-check> 
// Attribute: <ArnCovenant> Any-Architect <Environment> <Reference/Dependence>
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

#include "../../../inc/c/format/ELF.h"
#include "../../../inc/c/ustring.h"

unsigned _Literal_ELFBitmode[] = {
	0, 32, 64
};

const char* _Literal_OSABI[] = {
	"UNIX - System V",
	"Hewlett-Packard HP-UX",
	"NetBSD",
	"Linux",
	"Sun Solaris",
	"AIX",
	"IRIX",
	"FreeBSD",
	"Compaq TRU64 UNIX",
	"Novell Modesto",
	"Open BSD",
	"Open VMS",
	"Hewlett-Packard Non-Stop Kernel",
	"Amiga Research OS",
	"ARM EABI",
	"ARM",
	"Standalone (embedded applications)",
};

const char* _Literal_ELFType[] = {
	"NONE (No file type)",
	"REL (Relocatable)",
	"EXEC (Executable)",
	"DYN (Shared object)",
	"CORE (Core)",
	/*
	0xfe00 : "LOOS (OS specific)",
	0xfeff : "HIOS (OS specific)",
	0xff00 : "LOPROC (Processor specific)",
	0xffff : "HIPROC (Processor specific)",
	*/
};

const char* _Literal_ELFMachine[] = {
	"No machine",
	"AT&T WE 32100",
	"SPARC",
	"Intel 80386",
	"Motorola 68000",
	"Motorola 88000",
	"Reserved for future use (was EM_486)",
	"Intel 80860",
	"MIPS I Architecture",
	"IBM System/370 Processor",
	"MIPS RS3000 Little-endian",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Hewlett-Packard PA-RISC",
	"Reserved for future use",
	"Fujitsu VPP500",
	"Enhanced instruction set SPARC",
	"Intel 80960",
	"PowerPC",
	"64-bit PowerPC",
	"IBM System/390 Processor",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"NEC V800",
	"Fujitsu FR20",
	"TRW RH-32",
	"Motorola RCE",
	"Advanced RISC Machines ARM",
	"Digital Alpha",
	"Hitachi SH",
	"SPARC Version 9",
	"Siemens TriCore embedded processor",
	"Argonaut RISC Core, Argonaut Technologies Inc.",
	"Hitachi H8/300",
	"Hitachi H8/300H",
	"Hitachi H8S",
	"Hitachi H8/500",
	"Intel IA-64 processor architecture",
	"Stanford MIPS-X",
	"Motorola ColdFire",
	"Motorola M68HC12",
	"Fujitsu MMA Multimedia Accelerator",
	"Siemens PCP",
	"Sony nCPU embedded RISC processor",
	"Denso NDR1 microprocessor",
	"Motorola Star*Core processor",
	"Toyota ME16 processor",
	"STMicroelectronics ST100 processor",
	"Advanced Logic Corp. TinyJ embedded processor family",
	"AMD x86-64 architecture",
	"Sony DSP Processor",
	"Digital Equipment Corp. PDP-10",
	"Digital Equipment Corp. PDP-11",
	"Siemens FX66 microcontroller",
	"STMicroelectronics ST9+ 8/16 bit microcontroller",
	"STMicroelectronics ST7 8-bit microcontroller",
	"Motorola MC68HC16 Microcontroller",
	"Motorola MC68HC11 Microcontroller",
	"Motorola MC68HC08 Microcontroller",
	"Motorola MC68HC05 Microcontroller",
	"Silicon Graphics SVx",
	"STMicroelectronics ST19 8-bit microcontroller", // ?
	"Digital VAX",
	"Axis Communications 32-bit embedded processor",
	"Infineon Technologies 32-bit embedded processor",
	"Element 14 64-bit DSP Processor",
	"LSI Logic 16-bit DSP Processor",
	"Donald Knuth's educational 64-bit processor",
	"Harvard University machine-independent object files",
	"SiTera Prism",
	"Atmel AVR 8-bit microcontroller",
	"Fujitsu FR30",
	"Mitsubishi D10V",
	"Mitsubishi D30V",
	"NEC v850",
	"Mitsubishi M32R",
	"Matsushita MN10300",
	"Matsushita MN10200",
	"picoJava",
	"OpenRISC 32-bit embedded processor",
	"ARC Cores Tangent-A5",
	"Tensilica Xtensa Architecture",
	"Alphamosaic VideoCore processor",
	"Thompson Multimedia General Purpose Processor",
	"National Semiconductor 32000 series",
	"Tenor Network TPC processor",
	"Trebia SNP 1000 processor",
	"STMicroelectronics (www.st.com) ST200 microcontroller",
	"Ubicom IP2xxx microcontroller family",
	"MAX Processor",
	"National Semiconductor CompactRISC microprocessor",
	"Fujitsu F2MC16",
	"Texas Instruments embedded microcontroller msp430",
	"Analog Devices Blackfin (DSP) processor",
	"S1C33 Family of Seiko Epson processors",
	"Sharp embedded microprocessor",
	"Arca RISC Microprocessor",
	"Microprocessor series from PKU-Unity Ltd. and MPRC of Peking University",
};

static const char* _Literal_ELFProgramHeaderType[] = {
	"NULL",
	"LOAD",
	"DYNAMIC",
	"INTERP",
	"NOTE",
	"SHLIB",
	"PHDR",
	"TLS",
	"NUM",
	/*
	0x60000000 : "LOOS",
	0x6474e550 : "GNU_EH_FRAME",
	0x6474e551 : "GNU_STACK",
	0x6474e552 : "GNU_RELRO",
	0x6ffffffa : "LOSUNW",
	0x6ffffffc : "SUNWBSS",
	0x6ffffffb : "SUNWSTACK",
	0x6fffffff : "HISUNW ",
	0x6ffffffe : "HIOS",
	0x70000000 : "LOPROC",
	0x7fffffff : "HIPROC",
	0x70000001 : "ARM_EXIDX",
	0x70000002 : "ARM_PREEMPTMAP",
	0x70000003 : "ARM_ATTRIBUTES",
	0x70000004 : "ARM_DEBUGOVERLAY",
	0x70000005 : "ARM_OVERLAYSECTION",
	*/
};
const char* getLiteral_ELFProgramHeaderType(uint32 typ)
{
	if (typ < numsof(_Literal_ELFProgramHeaderType)) 
		return _Literal_ELFProgramHeaderType[typ];
	else if (typ == 0x60000000) return "LOOS";
	else if (typ == 0x6474e550) return "GNU_EH_FRAME";
	else if (typ == 0x6474e551) return "GNU_STACK";
	else if (typ == 0x6474e552) return "GNU_RELRO";
	else if (typ == 0x6ffffffa) return "LOSUNW";
	else if (typ == 0x6ffffffc) return "SUNWBSS";
	else if (typ == 0x6ffffffb) return "SUNWSTACK";
	else if (typ == 0x6fffffff) return "HISUNW";
	else if (typ == 0x6ffffffe) return "HIOS";
	else if (typ == 0x70000000) return "LOPROC";
	else if (typ == 0x7fffffff) return "HIPROC";
	else if (typ == 0x70000001) return "ARM_EXIDX";
	else if (typ == 0x70000002) return "ARM_PREEMPTMAP";
	else if (typ == 0x70000003) return "ARM_ATTRIBUTES";
	else if (typ == 0x70000004) return "ARM_DEBUGOVERLAY";
	else if (typ == 0x70000005) return "ARM_OVERLAYSECTION";
	return "UNKNOWN";
}

const char* _Literal_ELFProgramFlags[] = {
	"N..",
	"..E",
	".W.",
	".WE",
	"R..",
	"R.E",
	"RW.",
	"RWE",
};

static const char* _Literal_ELFSectionHeaderType[] = {
	"NULL", // Inactive section header 
	"PROGBITS", // Information defined by the program 
	"SYMTAB", // Symbol table - not DLL 
	"STRTAB", // String table `
	"RELA", // Explicit addend relocations, Elf64_Rela 
	"HASH", // Symbol hash table 
	"DYNAMIC", // Information for dynamic linking 
	"NOTE", // A Note section 
	"NOBITS", // Like SHT_PROGBITS with no data 
	"REL", // Implicit addend relocations, Elf64_Rel 
	"SHLIB", // Currently unspecified semantics 
	"?",
	"?",
	"DYNSYM", // Symbol table for a DLL 
	"INIT_ARRAY", // Array of constructors 
	"FINI_ARRAY", // Array of deconstructors 
	"PREINIT_ARRAY", // Array of pre-constructors 
	"GROUP", // Section group 
	"SYMTAB_SHNDX", // Extended section indeces 
	"NUM", // Number of defined types 
	/*
	0x60000000:"LOOS", // Lowest OS-specific section type
	0x6ffffff5:"GNU_ATTRIBUTES", // Object attribuytes
	0x6ffffff6:"GNU_HASH", // GNU-style hash table
	0x6ffffff7:"GNU_LIBLIST", // Prelink library list
	0x6ffffff8:"CHECKSUM", // Checksum for DSO content
	0x6ffffffa:"LOSUNW", // Sun-specific low bound
	0x6ffffffb:"SUNW_COMDAT",
	0x6ffffffc:"SUNW_syminfo",
	0x6ffffffd:"GNU_verdef", // Version definition section
	0x6ffffffe:"GNU_verdneed", // Version needs section
	0x6fffffff:"GNY_versym", // Version symbol table
	0x70000000:"LOPROC", // Start of processor-specific section type
	0x7fffffff:"HIPROC", // End of processor-specific section type
	0x80000000:"LOUSER", // Start of application-specific
	0x8fffffff:"HIUSER", // Ennd of application-specific
	*/
};
const char* getLiteral_ELFSectionHeaderType(uint32 typ)
{
	if (typ < numsof(_Literal_ELFSectionHeaderType))
		return _Literal_ELFSectionHeaderType[typ];
	else if (typ == 0x60000000) return "LOOS";
	else if (typ == 0x6ffffff5) return "GNU_ATTRIBUTES";
	else if (typ == 0x6ffffff6) return "GNU_HASH";
	else if (typ == 0x6ffffff7) return "GNU_LIBLIST";
	else if (typ == 0x6ffffff8) return "CHECKSUM";
	else if (typ == 0x6ffffffa) return "LOSUNW";
	else if (typ == 0x6ffffffb) return "SUNW_COMDAT";
	else if (typ == 0x6ffffffc) return "SUNW_syminfo";
	else if (typ == 0x6ffffffd) return "GNU_verdef";
	else if (typ == 0x6ffffffe) return "GNU_verdneed";
	else if (typ == 0x6fffffff) return "GNY_versym";
	else if (typ == 0x70000000) return "LOPROC";
	else if (typ == 0x7fffffff) return "HIPROC";
	else if (typ == 0x80000000) return "LOUSER";
	else if (typ == 0x8fffffff) return "HIUSER";
	return "UNKNOWN";
}

static const char* _Literal_ELFSectionFlags[] = {
	"",
	"W",
	"A",
	"WA",
	"",
	"",
	"AX",
	/*
	0x30: "MS",
	0x42: "AI",
	*/
};
const char* getLiteral_ELFSectionFlags(byte typ)
{
	if (typ < numsof(_Literal_ELFSectionFlags))
		return _Literal_ELFSectionFlags[typ];
	else if (typ == 0x30) return "MS";
	else if (typ == 0x42) return "AI";
	return "UNK"; // "UNKNOWN";
}

#if __BITS__ == 32
stduint ELF32_LoadExecFromMemory(const void* memsrc, void** p_entry)
{
	struct ELF_Header_t* header = (struct ELF_Header_t*)memsrc;
	stduint retval = 0;
	void* entry = (void*)header->e_entry;
	for0(i, header->e_phnum)
	{
		struct ELF_PHT_t* ph = (struct ELF_PHT_t*)((byte*)memsrc + header->e_phoff + header->e_phentsize * i);
		if (ph->p_type == PT_LOAD && (++retval))//{TEMP}VAddress
			MemCopyN((pureptr_t)ph->p_vaddr, (byte*)memsrc + ph->p_offset, ph->p_memsz); 
	}
	if (p_entry) *p_entry = entry;
	return retval;
}
#elif __BITS__ == 64
	//{TODO}
#endif

