; ASCII NASM TAB4 CRLF
; Attribute: CPU(I-80386) Mode(Real-16)
; LastCheck: 2024Feb13
; AllAuthor: @dosconio
; ModuTitle: File System - FAT12
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

; TODO  01 : If the buffer exists the needs, then use it but reload it;

; Read FAT12 File from Floppy Disk
; [Recommend ] Reset the disk before
; [Input<<<<<] PRES{ES, DS for fname} DestinationAddress, FileNameAddress(Len11), BufferAddress
; [Output    ] 0=No-Error-and-Found-File
; [CallConven] CDECL without sym-prefix

%include "floppy.a"
%include "INT13.a"; Disk BIOS
%include "string.a"

[BITS 16]; This is a must

DRV_ID EQU 0; FLOPPY DRIVE ID

GLOBAL ReadFileFFAT12

;EXTERN ReadFloppy;(...)

%define SeekPos (BP-2*1)
%define DestAdr (BP+2*2)
%define FnamAdr (BP+2*3)
%define FatBuf  (BP+2*4)

%macro ReadFlo 2-3 1;(%1 Dest, %2 SecId, %3 Times)
	%ifnidni %1, SI
		MOV SI, %1
	%endif
	%ifnidni %2, AX
		MOV AX, %2
	%endif
	MOV CL, %3
	CALL ReadFloppy
%endmacro

section .text
;[UPDATE]
; 20241112 FATBUF from ES to DS
ReadFileFFAT12:;<CDecl>(word DestinationAddress, word FileNameAddress, word BufferAddress)
	PUSH BP
	MOV BP, SP
	; [BP+2*0] = BP
	; [BP+2*1] = ReturnAddress
	; [BP+2*2] = ES: DestinationAddress
	; [BP+2*3] = DS: FileNameAddress
	; [BP+2*4] = DS: BufferAddress
	PUSH WORD (_FLOSEC_ROOT_START-1); [BP-2*1] SeekPos
	PUSHA
	;
	MOV CX, _FLOPPY_0144_BPB_ROE*32/512; _BPB_ROE * unitsize(32) / sectorsize(512) <=> EXPONENT: +5-9=-4
	OR CX, CX
	lup_seek:; under CX
		JZ Erro; from OR and JMP
		ADD WORD[SeekPos], 1
		PUSH CX
		CALL XchgSegs
		ReadFlo [FatBuf], [SeekPos]
		CALL XchgSegs
		; Compare Strings
			MOV SI, [FnamAdr]
			MOV DI, [FatBuf]
			MOV CX, 512/_FLOFAT_FILEENTO_LEN; 512/32
			lup_seek_compare:
				PUSH CX
				MOV CX, _FLOFAT_FILENAME_LEN
				JSTRX_DS load_file
				ADD DI, _FLOFAT_FILEENTO_LEN
				POP CX
				LOOP lup_seek_compare
		POP CX
		LOOP lup_seek
	load_file:
		; ADD SP, 2; Release Stack
		AND DI, 0xFFE0
		ADD DI, 0x001A; -> First Sector
		MOV BX, [DI]
		PUSH BX
		ADD BX, 14 + _FLOSEC_FAT1_START + _FLOSEC_FAT1_LEN + _FLOSEC_FAT2_LEN - 2; _BPB_ROE*32/512 + BootResv + ... - 2
		;
		MOV AX, BX
		MOV SI, [DestAdr]
		load_loop:
			ReadFlo SI, AX
			POP AX
			PUSHA
			CALL FATGetEntry
			JZ Back
			MOV BP, SP
			MOV WORD[BP+2*7], AX
			POPA
			PUSH AX
			ADD AX, 14 + _FLOSEC_FAT1_START + _FLOSEC_FAT1_LEN + _FLOSEC_FAT2_LEN - 2
			ADD SI, 512; _BPB_BPS
			JMP load_loop
XchgSegs:
	PUSH ES
	PUSH DS
	POP ES
	POP DS
	RET

FATGetEntry:;[Locale Procedure] [Volatile SI, BX, CX, DX, BP] [ZF:FileEnd, AX:NextSector(XXXH)]<<<[PreS ES=0][Input AX = Linear Sector Number]
	; PUSH CX>DX>BX>SI>BP>ES
	XOR CX, CX; TO HELP ALIGN
	MOV BX, 3
	MUL BX; DX`AX = 3 * AL, for a entry takes 12 bits
	SHR AX, 1
	JNC FATGetEntry_WhichSector
	OR CX, 4
	;
	FATGetEntry_WhichSector:
	PUSH CX
	XOR DX, DX
	MOV BX, 512; _BPB_BPS
	DIV BX; DX`AX/BX= AX:FAT-SECTOR ... DX:ENTRY-OFFSET
	;
	ADD AX, _FLOSEC_FAT1_START
	ReadFlo [FatBuf], AX, 2; 2 Sectors
	ADD SI, DX
	MOV AX, [ES:SI]
	POP CX
	SHR AX, CL
	AND AX, 0x0FFF
	;
	CMP AX, 0xFFF
	RET

Back:
	POPA
	MOV AX, 0
	MOV SP, BP
	POP BP
	RET

Erro:
	POPA
	MOV AX, 1
	MOV SP, BP
	POP BP
	RET

ReadFloppy:; PRES[ES] (CL:NoS, SI:Buf, AX:Lin)
	DiskReadSectors CL, SI, AX, DRV_ID
	RET

