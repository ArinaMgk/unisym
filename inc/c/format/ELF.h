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
};// Segment types

enum machine_t {
	EM_NONE = 0,
	EM_M32 = 1,
	EM_SPARC = 2,
	EM_386 = 3,
	EM_68K = 4,
	EM_88K = 5,
	EM_486 = 6, /* Not used in Linux at least */
	EM_860 = 7,
	EM_MIPS = 8, /* R3k, bigendian(?) */
	EM_MIPS_RS4_BE = 10, /* R4k BE */
	EM_PARISC = 15,
	EM_SPARC32PLUS = 18,
	EM_PPC = 20,
	EM_PPC64 = 21,
	EM_S390 = 22,
	EM_SH = 42,
	EM_SPARCV9 = 43, /* v9 = SPARC64 */
	EM_H8_300H = 47,
	EM_H8S = 48,
	EM_IA_64 = 50,
	EM_X86_64 = 62,
	EM_CRIS = 76,
	EM_V850 = 87,
	EM_ALPHA = 0x9026, /* Interrim Alpha that stuck around */
	EM_CYGNUS_V850 = 0x9080, /* Old v850 ID used by Cygnus */
	EM_S390_OLD = 0xA390, /* Obsolete interrim value for S/390 */
};

enum elf_dynamic_t {
	DT_NULL		= 0,
	DT_NEEDED	= 1,
	DT_PLTRELSZ	= 2,
	DT_PLTGOT	= 3,
	DT_HASH		= 4,
	DT_STRTAB	= 5,
	DT_SYMTAB	= 6,
	DT_RELA		= 7,
	DT_RELASZ	= 8,
	DT_RELAENT	= 9,
	DT_STRSZ	= 10,
	DT_SYMENT	= 11,
	DT_INIT		= 12,
	DT_FINI		= 13,
	DT_SONAME	= 14,
	DT_RPATH	= 15,
	DT_SYMBOLIC	= 16,
	DT_REL		= 17,
	DT_RELSZ	= 18,
	DT_RELENT	= 19,
	DT_PLTREL	= 20,
	DT_DEBUG	= 21,
	DT_TEXTREL	= 22,
	DT_JMPREL	= 23,
	DT_LOPROC	= 0x70000000,
	DT_HIPROC	= 0x7fffffff,
};

enum {
	AT_NULL		= 0	 ,/* end of vector */
	AT_IGNORE	= 1	 ,/* entry should be ignored */
	AT_EXECFD	= 2	 ,/* file descriptor of program */
	AT_PHDR		= 3	 ,/* program headers for program */
	AT_PHENT	= 4	 ,/* size of program header entry */
	AT_PHNUM	= 5	 ,/* number of program headers */
	AT_PAGESZ	= 6	 ,/* system page size */
	AT_BASE		= 7	 ,/* base address of interpreter */
	AT_FLAGS	= 8	 ,/* flags */
	AT_ENTRY	= 9	 ,/* entry point of program */
	AT_NOTELF	= 10 ,	/* program is not ELF */
	AT_UID		= 11 ,	/* real uid */
	AT_EUID		= 12 ,	/* effective uid */
	AT_GID		= 13 ,	/* real gid */
	AT_EGID		= 14 ,	/* effective gid */
	AT_PLATFORM	= 15 ,	/* string identifying CPU for optimizations */
	AT_HWCAP  	= 16 ,	/* arch dependent hints at CPU capabilities */
	AT_CLKTCK 	= 17 ,	/* frequency at which times() increments */
	AT_SECURE 	= 23 ,	/* secure mode boolean */
};// Auxilliary table entries

enum {
	PF_X = 0x1,
	PF_W = 0x2,
	PF_R = 0x4,
};// Program header permission flags

enum {
	SHT_NULL     = 0,
	SHT_PROGBITS = 1,
	SHT_SYMTAB   = 2,
	SHT_STRTAB   = 3,
	SHT_RELA     = 4,
	SHT_HASH     = 5,
	SHT_DYNAMIC  = 6,
	SHT_NOTE     = 7,
	SHT_NOBITS   = 8,
	SHT_REL      = 9,
	SHT_SHLIB    = 10,
	SHT_DYNSYM   = 11,
	SHT_NUM      = 12,
	SHT_LOPROC   = 0x70000000,
	SHT_HIPROC   = 0x7fffffff,
	SHT_LOUSER   = 0x80000000,
	SHT_HIUSER   = 0xffffffff,
};// Section header types

enum {
	SHF_WRITE             = _IMM1S(0) ,  /* Writable */
	SHF_ALLOC             = _IMM1S(1) ,  /* Occupies memory during execution */
	SHF_EXECINSTR         = _IMM1S(2) ,  /* Executable */
	SHF_MERGE             = _IMM1S(4) ,  /* Might be merged */
	SHF_STRINGS           = _IMM1S(5) ,  /* Contains nul-terminated strings */
	SHF_INFO_LINK         = _IMM1S(6) ,  /* `sh_info' contains SHT index */
	SHF_LINK_ORDER        = _IMM1S(7) ,  /* Preserve order after combining */
	SHF_OS_NONCONFORMING  = _IMM1S(8) ,  /* Non-standard OS specific handling required */
	SHF_GROUP             = _IMM1S(9) ,  /* Section is member of a group.  */
	SHF_TLS               = _IMM1S(10),  /* Section hold thread-local data.  */
};// Section header flags

/* Special section numbers */
#define SHN_UNDEF       0
#define SHN_LORESERVE   0xff00
#define SHN_LOPROC      0xff00
#define SHN_HIPROC      0xff1f
#define SHN_ABS         0xfff1
#define SHN_COMMON      0xfff2
#define SHN_HIRESERVE   0xffff

/* Magic number constants... */
#define EI_MAG0         0	/* e_ident[] indexes */
#define EI_MAG1         1
#define EI_MAG2         2
#define EI_MAG3         3
#define EI_CLASS        4
#define EI_DATA         5
#define EI_VERSION      6
#define EI_OSABI        7
#define EI_PAD          8

#define ELFMAG0         0x7f	/* EI_MAG */
#define ELFMAG1         'E'
#define ELFMAG2         'L'
#define ELFMAG3         'F'
#define ELFMAG          "\177ELF"
#define SELFMAG         4

#define ELFCLASSNONE    0	/* EI_CLASS */
#define ELFCLASS32      1
#define ELFCLASS64      2
#define ELFCLASSNUM     3

#define ELFDATANONE     0	/* e_ident[EI_DATA] */
#define ELFDATA2LSB     1
#define ELFDATA2MSB     2

#define EV_NONE         0	/* e_version, EI_VERSION */
#define EV_CURRENT      1
#define EV_NUM          2

#define ELFOSABI_NONE   0
#define ELFOSABI_LINUX  3

/* Legal values for ST_BIND subfield of st_info (symbol binding).  */
#define STB_LOCAL	0		/* Local symbol */
#define STB_GLOBAL	1		/* Global symbol */
#define STB_WEAK	2		/* Weak symbol */
#define STB_NUM		3		/* Number of defined types.  */
#define STB_LOOS	10		/* Start of OS-specific */
#define STB_HIOS	12		/* End of OS-specific */
#define STB_LOPROC	13		/* Start of processor-specific */
#define STB_HIPROC	15		/* End of processor-specific */

/* Symbol types */
#define STT_NOTYPE	0		/* Symbol type is unspecified */
#define STT_OBJECT	1		/* Symbol is a data object */
#define STT_FUNC	2		/* Symbol is a code object */
#define STT_SECTION	3		/* Symbol associated with a section */
#define STT_FILE	4		/* Symbol's name is file name */
#define STT_COMMON	5		/* Symbol is a common data object */
#define STT_TLS		6		/* Symbol is thread-local data object*/
#define	STT_NUM		7		/* Number of defined types.  */

/* Symbol visibilities */
#define STV_DEFAULT     0               /* Default symbol visibility rules */
#define STV_INTERNAL    1               /* Processor specific hidden class */
#define STV_HIDDEN      2               /* Sym unavailable in other modules */
#define STV_PROTECTED   3               /* Not preemptible, not exported */

// ---- ---- ---- ---- //

#define SYM_GLOBAL 0x10

#define GLOBAL_TEMP_BASE  0x40000000 /* bigger than any sane symbol index */

#define SEG_ALIGN 16            /* alignment of sections in file */
#define SEG_ALIGN_1 (SEG_ALIGN-1)

/* this stuff is needed for the stabs debugging format */
#define N_SO 0x64               /* ID for main source file */
#define N_SOL 0x84              /* ID for sub-source file */
#define N_BINCL 0x82
#define N_EINCL 0xA2
#define N_SLINE 0x44
#define TY_STABSSYMLIN 0x40     /* ouch */

/* this stuff is needed for the dwarf debugging format */
#define TY_DEBUGSYMLIN 0x40     /* internal call to debug_out */

/* Known sections with nonstandard defaults */
struct elf_known_section {
    const char *name;		/* Name of section */
    int type;			/* Section type (SHT_) */
    uint32_t flags;		/* Section flags (SHF_) */
    uint32_t align;		/* Section alignment */
};

/*
 * Special ELF sections (after the real sections but before debugging ones)
 */
#define sec_shstrtab    (nsects + 1)
#define sec_symtab      (nsects + 2)
#define sec_strtab      (nsects + 3)
#define sec_numspecial  3

/*
 * Debugging ELF sections (last in the file)
 */

/* stabs */
#define sec_stab		(nsections-3)
#define sec_stabstr		(nsections-2)
#define sec_rel_stab		(nsections-1)

/* dwarf */
#define sec_debug_aranges      (nsections-10)
#define sec_rela_debug_aranges (nsections-9)
#define sec_debug_pubnames     (nsections-8)
#define sec_debug_info         (nsections-7)
#define sec_rela_debug_info    (nsections-6)
#define sec_debug_abbrev       (nsections-5)
#define sec_debug_line         (nsections-4)
#define sec_rela_debug_line    (nsections-3)
#define sec_debug_frame        (nsections-2)
#define sec_debug_loc          (nsections-1)


// ---- ---- ---- ---- //

#define EI_NIDENT 16 // Length of magic at the start of a file
#ifndef ELFBIT
	#define ELFBIT __BITS__
#endif

#define ELF32_ST_BIND(val)		(((unsigned char) (val)) >> 4)
#define ELF32_ST_TYPE(val)		((val) & 0xf)

#if ELFBIT == 32
	typedef uint32   Elf32_Addr;
	typedef uint32   Elf32_Off;
	typedef uint16_t Elf32_Section;
	//
	typedef uint16   Elf32_Half;
	typedef int16_t  Elf32_SHalf;
	typedef sint32   Elf32_Sword;
	typedef uint32   Elf32_Word;
	typedef uint64_t Elf32_Xword;
	typedef int64_t  Elf32_Sxword;
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
	typedef struct ELF_PHT_t {
		Elf32_Word p_type  ; //{TODO}
		Elf32_Off  p_offset; // offset of segment in file
		Elf32_Addr p_vaddr ; // virtual address of segment in memory
		Elf32_Addr p_paddr ; // physical address of segment in memory
		Elf32_Word p_filesz; // size of segment in file
		Elf32_Word p_memsz ; // size of segment in memory
		Elf32_Word p_flags ; // [?READ ?WRITE ?EXEC]
		Elf32_Word p_align ; // the alignment of segment in memory
	} Elf32_Phdr;
	

	typedef struct ELF_SHT_t {
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
	} Elf32_Shdr;
	// PHT [e_phoff ~ e_phoff + e_phnum*e_phentsize *] (suf-* means not including)

	typedef struct elf32_dyn {
		Elf32_Sword d_tag;
		union {
		Elf32_Sword d_val;
		Elf32_Addr d_ptr;
		} d_un;
	} Elf32_Dyn;// Dynamic header

	// Relocations

	#define ELF32_R_SYM(x)	((x) >> 8)
	#define ELF32_R_TYPE(x)	((x) & 0xff)

	typedef struct elf32_rel {
		Elf32_Addr r_offset;
		Elf32_Word r_info;
	} Elf32_Rel;

	typedef struct elf32_rela {
		Elf32_Addr r_offset;
		Elf32_Word r_info;
		Elf32_Sword r_addend;
	} Elf32_Rela;

	enum reloc32_type {
		R_386_32		=  1,   /* ordinary absolute relocation */
		R_386_PC32          =  2,   /* PC-relative relocation */
		R_386_GOT32         =  3,   /* an offset into GOT */
		R_386_PLT32         =  4,   /* a PC-relative offset into PLT */
		R_386_COPY          =  5,   /* ??? */
		R_386_GLOB_DAT      =  6,   /* ??? */
		R_386_JUMP_SLOT     =  7,   /* ??? */
		R_386_RELATIVE      =  8,   /* ??? */
		R_386_GOTOFF        =  9,   /* an offset from GOT base */
		R_386_GOTPC         = 10,   /* a PC-relative offset _to_ GOT */
		R_386_TLS_TPOFF     = 14,   /* Offset in static TLS block */
		R_386_TLS_IE        = 15,   /* Address of GOT entry for static TLS
									block offset */
		/* These are GNU extensions, but useful */
		R_386_16            = 20,   /* A 16-bit absolute relocation */
		R_386_PC16          = 21,   /* A 16-bit PC-relative relocation */
		R_386_8             = 22,   /* An 8-bit absolute relocation */
		R_386_PC8           = 23    /* An 8-bit PC-relative relocation */
	};

	typedef struct elf32_sym {
		Elf32_Word st_name;
		Elf32_Addr st_value;
		Elf32_Word st_size;
		unsigned char st_info;
		unsigned char st_other;
		Elf32_Half st_shndx;
	} Elf32_Sym;

	typedef struct elf32_note {
		Elf32_Word n_namesz;	/* Name size */
		Elf32_Word n_descsz;	/* Content size */
		Elf32_Word n_type;	/* Content type */
	} Elf32_Nhdr;// Note header


	

#elif ELFBIT == 64

	typedef uint64_t  Elf64_Addr;// uintptr_t
	typedef uint64_t  Elf64_Off;
	typedef uint16_t  Elf64_Section;
	//
	typedef uint16_t  Elf64_Half;
	typedef uint32_t  Elf64_Word;
	typedef int32_t   Elf64_Sword;
	typedef uint64_t  Elf64_Xword;
	typedef int64_t   Elf64_Sxword;

	struct ELF_Header_t {
		unsigned char e_ident[EI_NIDENT];
		Elf64_Half    e_type;
		Elf64_Half    e_machine;
		Elf64_Word    e_version;
		Elf64_Addr    e_entry;
		Elf64_Off     e_phoff;
		Elf64_Off     e_shoff;
		Elf64_Word    e_flags;
		Elf64_Half    e_ehsize;
		Elf64_Half    e_phentsize;
		Elf64_Half    e_phnum;
		Elf64_Half    e_shentsize;
		Elf64_Half    e_shnum;
		Elf64_Half    e_shstrndx;
	};
	typedef struct ELF_Header_t Elf64_Ehdr;

	struct ELF_PHT_t {
		Elf64_Word  p_type;
		Elf64_Word  p_flags;
		Elf64_Off   p_offset;
		Elf64_Addr  p_vaddr;
		Elf64_Addr  p_paddr;
		Elf64_Xword p_filesz;
		Elf64_Xword p_memsz;
		Elf64_Xword p_align;
	};
	typedef struct ELF_PHT_t Elf64_Phdr;

	typedef struct elf64_shdr {
		Elf64_Word sh_name;
		Elf64_Word sh_type;
		Elf64_Xword sh_flags;
		Elf64_Addr sh_addr;
		Elf64_Off sh_offset;
		Elf64_Xword sh_size;
		Elf64_Word sh_link;
		Elf64_Word sh_info;
		Elf64_Xword sh_addralign;
		Elf64_Xword sh_entsize;
	} Elf64_Shdr;// Section header

	typedef struct elf64_note {
		Elf64_Word n_namesz;	/* Name size */
		Elf64_Word n_descsz;	/* Content size */
		Elf64_Word n_type;	/* Content type */
	} Elf64_Nhdr;// Note header

	typedef struct {
	Elf64_Sxword d_tag;
		union {
			Elf64_Xword d_val;
			Elf64_Addr d_ptr;
		} d_un;
	} Elf64_Dyn;// Dynamic header

	#define DT_NULL    0
	#define DT_RELA    7
	#define DT_RELASZ  8
	#define DT_RELAENT 9

	// Relocations

	typedef struct elf64_rel {
		Elf64_Addr r_offset;
		Elf64_Xword r_info;
	} Elf64_Rel;

	typedef struct {
		Elf64_Addr r_offset;
		Elf64_Xword r_info;
		Elf64_Sxword r_addend;
	} Elf64_Rela;


	#define ELF64_R_SYM(i)    ((i)>>32)
	#define ELF64_R_TYPE(i)   ((i)&0xffffffffL)
	#define ELF64_R_INFO(s,t) (((s)<<32)+((t)&0xffffffffL))

	enum reloc64_type {
		R_X86_64_NONE       =  0,	/* No reloc */
		R_X86_64_64	        =  1,	/* Direct 64 bit  */
		R_X86_64_PC32	=  2,  	/* PC relative 32 bit signed */
		R_X86_64_GOT32	=  3,	/* 32 bit GOT entry */
		R_X86_64_PLT32	=  4,	/* 32 bit PLT address */
		R_X86_64_COPY	=  5,	/* Copy symbol at runtime */
		R_X86_64_GLOB_DAT	=  6,	/* Create GOT entry */
		R_X86_64_JUMP_SLOT	=  7,	/* Create PLT entry */
		R_X86_64_RELATIVE	=  8,	/* Adjust by program base */
		R_X86_64_GOTPCREL	=  9,	/* 32 bit signed PC relative offset to GOT */
		R_X86_64_32		= 10,	/* Direct 32 bit zero extended */
		R_X86_64_32S	= 11,	/* Direct 32 bit sign extended */
		R_X86_64_16		= 12,	/* Direct 16 bit zero extended */
		R_X86_64_PC16	= 13,	/* 16 bit sign extended pc relative */
		R_X86_64_8		= 14,	/* Direct 8 bit sign extended  */
		R_X86_64_PC8	= 15,	/* 8 bit sign extended pc relative */
		R_X86_64_DTPMOD64	= 16,	/* ID of module containing symbol */
		R_X86_64_DTPOFF64	= 17,	/* Offset in module's TLS block */
		R_X86_64_TPOFF64	= 18,	/* Offset in initial TLS block */
		R_X86_64_TLSGD	= 19,	/* 32 bit signed PC relative offset to two GOT entries for GD symbol */
		R_X86_64_TLSLD	= 20,	/* 32 bit signed PC relative offset to two GOT entries for LD symbol */
		R_X86_64_DTPOFF32	= 21,	/* Offset in TLS block */
		R_X86_64_GOTTPOFF	= 22,	/* 32 bit signed PC relative offset to GOT entry for IE symbol */
		R_X86_64_TPOFF32	= 23,	/* Offset in initial TLS block */
		R_X86_64_PC64	= 24, 	/* word64 S + A - P */
		R_X86_64_GOTOFF64	= 25, 	/* word64 S + A - GOT */
		R_X86_64_GOTPC32	= 26, 	/* word32 GOT + A - P */
		R_X86_64_GOT64	= 27, 	/* word64 G + A */
		R_X86_64_GOTPCREL64	= 28, 	/* word64 G + GOT - P + A */
		R_X86_64_GOTPC64	= 29, 	/* word64 GOT - P + A */
		R_X86_64_GOTPLT64	= 30, 	/* word64 G + A */
		R_X86_64_PLTOFF64	= 31, 	/* word64 L - GOT + A */
		R_X86_64_SIZE32	= 32, 	/* word32 Z + A */
		R_X86_64_SIZE64	= 33, 	/* word64 Z + A */
		R_X86_64_GOTPC32_TLSDESC = 34, 	/* word32 */
		R_X86_64_TLSDESC_CALL    = 35, 	/* none */
		R_X86_64_TLSDESC    = 36 	/* word64×2 */
	};

	// Symbol
	typedef struct elf64_sym {
		Elf64_Word st_name;
		unsigned char st_info;
		unsigned char st_other;
		Elf64_Half st_shndx;
		Elf64_Addr st_value;
		Elf64_Xword st_size;
	} Elf64_Sym;

	#define ELF64_ST_BIND(val) ELF32_ST_BIND(val)
	#define ELF64_ST_TYPE(val) ELF32_ST_TYPE(val)

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

#ifdef _INC_CPP
extern "C" {
#endif


// ELF32_LoadFromMemory //{TODO} ELF<32>::LoadFromMemory
//{TEMP} no check for validation
// return how many PHBlock has been loaded
stduint ELF32_LoadExecFromMemory(const void* memsrc, void** p_entry);
stduint ELF64_LoadExecFromMemory(const void* memsrc, void** p_entry);

#ifdef _INC_CPP
}
#endif

#endif
