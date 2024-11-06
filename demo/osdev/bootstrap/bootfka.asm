; ASCII NASM TAB4 CRLF
; Attribute: CPU(I-80386) DESTOPT(_FLOPPY)
; LastCheck: 2024Feb11
; AllAuthor: @dosconio
; ModuTitle: Bootstrap (MBR) for Mecocoa
; Copyright: ArinaMgk UniSym, Apache License Version 2.0
%ifdef _BASE_DOS
	BOOT_ENTRY  EQU 0x1000
%else
BOOT_ENTRY  EQU 0x7C00
%endif
ADDR_STACK  EQU 0x0600
FAT_BUFFER  EQU 0x0600;~ 0x07FF
ADDR_KERELF EQU 0x1000
ADDR_KERNEL EQU 0x5000

%ifdef _FLOPPY
	DRV_ID EQU 0x00
%else
	DRV_ID EQU 0x80
%endif

[CPU 386]

%include "pseudo.a"
%include "floppy.a"
%include "video.a"
%include "demos.a"
%include "debug.a"
%include "string.a"
%include "INT10.a"; Video BIOS
%include "INT13.a"; Disk BIOS
%include "INT16.a"; Keyboard BIOS

HeadFloppyFAT12 'MECOCOA SYS'; suits for any disk for jump-instruction

DefineStack16 0x0000, ADDR_STACK
MOV AX, BOOT_ENTRY/0x10
MOV DS, AX
XOR AX, AX
MOV ES, AX

MOV SI, str
CALL Print
KeyRead

; LOAD KERNEL-16
%ifdef _FLOPPY; ES -> 0x0000
	DiskReset
	; SEEK KER.APP
	MOV BP, [_BPB_ROE]
	MOV CL, 4; _BPB_ROE * unitsize(32) / sectorsize(512) <=> EXPONENT: +5-9=-4
	SHR BP, CL
	PUSH WORD (_FLOSEC_ROOT_START-1)
	lup_seek:; under BP
		JZ Erro; from SHL and JMP
		;
		POP AX
		INC AX; WHERE TO SEEK
		PUSH AX
		;
		MOV SI, FAT_BUFFER
		MOV CL, 1
		CALL ReadFloppy
		; Compare Strings
			MOV SI, kernel_iden
			MOV DI, FAT_BUFFER
			MOV CX, 512/_FLOFAT_FILEENTO_LEN; 512/32
			lup_seek_compare:
				PUSH CX
				MOV CX, _FLOFAT_FILENAME_LEN
				JSTRX load_file
				ADD DI, _FLOFAT_FILEENTO_LEN
				POP CX
				LOOP lup_seek_compare
		DEC BP
		JMP lup_seek
	load_file:
		ADD SP, 2; Release BP
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
			CALL ReadFloppy
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
	FATGetEntry:;[Locale Procedure] [Volatile SI, BX, CX, DX, BP] [ZF:FileEnd, AX:NextSector(XXXH)]<<<[PreS ES=0][Input AX = Linear Sector Number]
		; PUSH CX>DX>BX>SI>BP>ES
		XOR BP, BP; TO HELP ALIGN
		; MOV ES, BP
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
		CALL ReadFloppy
		ADD SI, DX
		MOV AX, [ES:SI]
		MOV CX, BP
		SHR AX, CL
		AND AX, 0x0FFF; ...Debug for 2h 2024Feb01 00:00
		;
		CMP AX, 0xFFF
		RET
	ReadFloppy:; PRES[ES] (CL:NoS, SI:Buf, AX:Lin)
		DiskReadSectors CL, SI, AX, DRV_ID
		RET
	load_endo:
%else
	PUSH ES
	CodefileLoad16 ADDR_KERELF, 2, 2; Sector 02 -> ES:0
	POP ES
%endif

; FLAT OR ELF {TODO SCRIPT AND OTHERS} - (DS=0000)
PUSH ES
POP DS
;CMP BYTE[ADDR_KERELF+0xF], 'F'
;JNZ JMPK; TREAT AS FLAT, NO CONSIDERATION FOR STACK
MOV BX, [ADDR_KERELF+0x18]; ENTRY
PUSH WORD 0; CS
PUSH BX    ; IP
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
	REP MOVSB
	lup_loadkernel_next0: POPA
lup_loadkernel_next:
	ADD  BX, 0x20; SIZEOF PHT
	LOOP lup_loadkernel

; JUMP TO KERNEL-16
JMPKF:
	RETF ;JMP 0x0500: OFFSET
JMPK:
	JMP 0x0100: 0
Endo:DbgStop;{TEMP} NO RETURN PARENT-DOS
Erro:
	MOV SI, error
	CALL Print
	JMP Endo
Print:
	PrintStringCursor SI
	RET
; ---- DATA ----
str:
	DB "Press any key to enter Mecocoa"
	%ifdef _FLOPPY
	DB "(F)"; Floppy
	%else
	DB "(D)"; Flat Disk
	%endif
	DB "...",10,13,0; Mecocoa Style LFCR for new-line
error:
	DB "No Script or Kernel Found!",10,13,0
kernel_iden:
	DB "KER     APP"
; DB "DOSCONIO"
; ---- FILE END ----
TIMES 510-$+$$ DB 0
%ifdef __AASM__
	ARINA
%else
	DB 01010101B, 10101010B; IS EQUIVALENT TO ARINA INSTRUCTION
%endif
