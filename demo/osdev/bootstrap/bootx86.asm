; Docutitle: (Bootstrap) for x86
; Codifiers: @dosconio: 20240218 ~ 1111 
; Attribute: Arn-Covenant i386+ Env-Freestanding Non-Dependence BIOS/MBR
; Copyright: UNISYM, under Apache License 2.0
; Descriptn: Popular Bootstrap for ATX-PC is UEFI now

%ifdef _BASE_DOS
	BOOT_ENTRY  EQU 0x1000
%else; BIOS
	BOOT_ENTRY  EQU 0x7C00
%endif
ADDR_STACK  EQU 0x0600
FAT_BUFFER  EQU 0x0600;~ 0x07FF
ADDR_KERELF EQU 0x7E00
ADDR_KERNEL EQU 0x0200
%ifdef _FLOPPY
	DRV_ID EQU 0x00
%else; HARDDISK
	DRV_ID EQU 0x80
%endif

;{TODO Solve} Load from Floppy or Harddisk ?

[CPU 386]

%include "pseudo.a"
%include "floppy.a"
%include "video.a"
%include "demos.a"
%include "debug.a"
%include "osdev.a"
%include "string.a"
%include "INT10.a"; Video BIOS
%include "INT13.a"; Disk BIOS
;%include "INT16.a"; Keyboard BIOS

HeadFloppyFAT12 {'MECOCOASYS', DRV_ID}

DefineStack16 0x0000, ADDR_STACK
MOV AX, BOOT_ENTRY/0x10
MOV DS, AX
XOR AX, AX
MOV ES, AX
; LOAD KERNEL
	DiskReset
	; SEEK KER.APP for REAL16 and KEX.APP for FLAP32
	MOV BP, [_BPB_ROE]
	MOV CL, 4; _BPB_ROE * unitsize(32) / sectorsize(512) <=> EXPONENT: +5-9=-4
	SHR BP, CL
	PUSH WORD (_FLOSEC_ROOT_START-1)
	lup_seek:; under BP
		JZ Erro; number of BP (entries)
		;
		POP AX
		INC AX; WHERE TO SEEK
		PUSH AX
		;
		MOV SI, FAT_BUFFER
		MOV CL, 1
		CALL ReadXdisk
		; Compare Strings
			CALL lup_seek_compare_call; Check "KER "
			MOV BYTE[kernel_iden+2], 'X'
			CALL lup_seek_compare_call; Check "KEX "
			MOV BYTE[kernel_iden+2], 'R'
			lup_seek_compare_call:
			MOV SI, kernel_iden
			MOV DI, FAT_BUFFER
			MOV CX, 512/_FLOFAT_FILEENTO_LEN; 512/32
			lup_seek_compare:
				PUSH CX
				MOV CX, 4+1; _FLOFAT_FILENAME_LEN
				JSTRX load_file
				ADD DI, _FLOFAT_FILEENTO_LEN
				POP CX
				LOOP lup_seek_compare
				RET
		DEC BP
		JMP lup_seek
	load_file:
		ADD SP, 2*2; Release BP
		AND DI, 0xFFE0
		ADD DI, 0x001A; -> First Sector
		MOV BX, [ES:DI]
		PUSH BX
		ADD BX, 14 + _FLOSEC_FAT1_START + _FLOSEC_FAT1_LEN + _FLOSEC_FAT2_LEN - 2; _BPB_ROE*32/512 + BootResv + ... - 2
		;
		MOV AX, BX
		MOV CL, 1
		MOV SI, ADDR_KERELF; ...Debug for 2h
		load_loop:
			CALL ReadXdisk
			POP AX
			PUSHA
			CALL FATGetEntry
			JZ load_endo
			MOV BP, SP
			MOV WORD[BP+2*7], AX
			POPA
			PUSH AX
			ADD AX, 14 + _FLOSEC_FAT1_START + _FLOSEC_FAT1_LEN + _FLOSEC_FAT2_LEN - 2
			ADD SI, 512; _BPB_BPS
			JMP load_loop
	FATGetEntry:
		XOR BP, BP; TO HELP ALIGN
		MOV BX, 3
		MUL BX; DX`AX = 3 * AL, for a entry takes 12 bits
		SHR AX, 1
		JNC FATGetEntry_WhichSector
		OR BP, 4
		;
		FATGetEntry_WhichSector:
		XOR DX, DX
		MOV BX, 512; _BPB_BPS
		DIV BX; DX`AX/BX= AX:FAT-SECTOR ... DX:ENTRY-OFFSET
		;
		ADD AX, _FLOSEC_FAT1_START
		MOV SI, FAT_BUFFER
		MOV CL, 2
		CALL ReadXdisk
		ADD SI, DX
		MOV AX, [ES:SI]
		MOV CX, BP
		SHR AX, CL
		AND AX, 0x0FFF; ...Debug for 2h 2024Feb01 00:00
		;
		CMP AX, 0xFFF
		RET
	ReadXdisk:; PRES[ES, DL] (CL:NoS, SI:Buf, AX:Lin) Vola(DX)
		PUSH DX
		DiskReadSectors CL, SI, AX, [_BS_Vols + 10]; [1]AUTO from hdisk/floppy/...
		POP DX
		RET
	load_endo:

; FLAT OR ELF {TODO SCRIPT AND OTHERS} - (DS=0000 after loaded)
	PUSH ES
	POP DS
	MOV BX, [ADDR_KERELF+0x18]; ENTRY
	MOVZX EBX, BX
	PUSH EBX    ; EIP
	MOV BX, [ADDR_KERELF+0x1C]; ELF HEADER
	ADD BX, ADDR_KERELF
	MOV CX, [ADDR_KERELF+0x2C]; NUMOF PHT
	CLD
	lup_loadkernel:
		CMP DWORD[BX], 0
		JZ  lup_loadkernel_next; NO LOAD
		PUSHA
		MOV ECX, DWORD[BX+0x10]; ABANDON HIGH 16-BITS
		MOV ESI, DWORD[BX+0x04]; ABANDON HIGH 16-BITS
		ADD ESI, ADDR_KERELF; OFFSET
		MOV EDI, DWORD[BX+0x08]
		CMP EDI, ADDR_KERNEL
		JB  lup_loadkernel_next0
		REP MOVSB; assert ECX + EDI < 0x7c00
		lup_loadkernel_next0: POPA
	lup_loadkernel_next:
		ADD  BX, 0x20; SIZEOF PHT
		LOOP lup_loadkernel
; ENTER FLAT-32
	CMP BYTE[BOOT_ENTRY+kernel_iden+2], 'R'; [2]AUTO KER detected, All SegRegs are 0
	JZ Retshort
	EnterFlat: CLI
	LGDT [ES:0x7c00+gdt]
	Addr20Enable
	MOV EAX, CR0
	OR EAX, 1
	MOV CR0, EAX
	JMP 8*2:0x7c00+JMPKF
; FUNCTIONS
Erro:
	MOV SI, error
	CALL Print
	DbgStop
Print:
	PrintStringCursor SI
Retshort:
	RET
[BITS 32]
; JUMP TO KERNEL
JMPKF:
	MOV EAX, 8*1
	MOV DS, AX
	MOV ES, AX
	MOV FS, AX
	MOV GS, AX
	MOV SS, AX
	MOVZX ESP, SP
	RET

; ---- DATA ----
gdt:
	DW 8*3-1
	DD 0x7c00+gdtsptrs
gdtsptrs:
	DD 0,0; SS cannot be 0
	DQDptr; Data and Stack
	DQDptr 0x00000000, 0xFFFFF, 0x00CF9A00; Code
error: DB "!",0
kernel_iden: DB "KER "
; ---- FILE END ----
TIMES 510-$+$$ DB 0
ARINA
