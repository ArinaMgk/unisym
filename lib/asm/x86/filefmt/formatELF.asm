; ASCII NASM TAB4 CRLF
; Attribute: CPU(I-80386)
; LastCheck: 2024Feb14
; AllAuthor: @dosconio
; ModuTitle: File Format - ELF
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

; Load (Copy) Segments from Memory
; [Input<<<<<] PRES{ES} ELFAddress
; [Output    ] EntryAddress

[CPU 386]
[BITS 16]

GLOBAL LoadFileMELF
GLOBAL _LoadFileMELF

; "M" is from memory
LoadFileMELF:
	PUSH BP
	MOV BP, SP
	; [BP-2*1] = AX
	; [BP+2*0] = BP
	; [BP+2*1] = ReturnAddress
	; [BP+2*2] = ELFAddress
	PUSHA; local variables
	MOV SI, [BP+2*2]; ELFAddress
	; CMP BYTE[SI+0x0F], 'F'
	; JNZ JMPK; TREAT AS FLAT, NO CONSIDERATION FOR STACK
	MOV BX, [ES:SI+0x18]; ENTRY
	MOV [   BP-2*01], BX; IP
	MOV BX, [ES:SI+0x1C]; ELF HEADER
	ADD BX, SI
	MOV CX, [ES:SI+0x2C]; NUMOF PHT
	CLD
	lupx_loadkernel:
		CMP DWORD[ES:BX], 0
		JZ  lupx_loadkernel_next; NO LOAD
		PUSHA
		MOV CX, [ES:BX+0x10]; ABANDON HIGH 16-BITS
		MOV SI, [ES:BX+0x04]; ABANDON HIGH 16-BITS
		ADD SI, [   BP+2*02]; OFFSET
		MOV DI, [ES:BX+0x08]
		CMP DI, 0;ADDR_KERNEL
		JBE  lupx_loadkernel_next0
		REP MOVSB
		lupx_loadkernel_next0:
		POPA
	lupx_loadkernel_next:
		ADD  BX, 0x20; SIZEOF PHT
		LOOP lupx_loadkernel
	;
	POPA
	MOV SP, BP
	POP BP
	RET

[BITS 32]
; since ELF32_LoadExecFromMemory exists, this was changed into MACRO since ELF upgrade to .c(pp)/m file
_LoadFileMELF:
	PUSH EBP
	MOV EBP, ESP
	; [EBP-4*1] = EAX
	; [EBP+4*0] = EBP
	; [EBP+4*1] = ReturnAddress
	; [EBP+4*2] = ELFAddress
	PUSHAD; local variables
	MOV ESI, [EBP+4*2]; ELFAddress
	; CMP BYTE[SI+0x0F], 'F'
	; JNZ JMPK; TREAT AS FLAT, NO CONSIDERATION FOR STACK
	MOV EBX, [ES:ESI+0x18]; ENTRY
	MOV [   EBP-4*01], EBX; IP
	MOV EBX, [ES:ESI+0x1C]; ELF HEADER
	ADD EBX, ESI
	MOV CX, [ES:ESI+0x2C]; NUMOF PHT
	MOVZX ECX, CX
	CLD
	lup_loadkernel:
		CMP DWORD[ES:EBX], 0
		JZ  lup_loadkernel_next; NO LOAD
		PUSHAD
		MOV ECX, [ES:EBX+0x10]; ABANDON HIGH 16-BITS
		MOV ESI, [ES:EBX+0x04]; ABANDON HIGH 16-BITS
		ADD ESI, [   EBP+4*02]; OFFSET
		MOV EDI, [ES:EBX+0x08]
		CMP EDI, 0;ADDR_KERNEL
		JBE  lup_loadkernel_next0
		REP MOVSB
		lup_loadkernel_next0:
		POPAD
	lup_loadkernel_next:
		ADD  EBX, 0x20; SIZEOF PHT
		LOOP lup_loadkernel
	;
	POPAD
	MOV ESP, EBP
	POP EBP
	RET


