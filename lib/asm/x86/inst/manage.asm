; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86) Format(ELF)
; LastCheck: 2024Feb18
; AllAuthor: @dosconio
; ModuTitle: 
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

GLOBAL _HALT
GLOBAL _InterruptEnable, _InterruptDisable, _InterruptDTabLoad
GLOBAL _getCR3, _getEflags
GLOBAL _jmpFar, _CallFar
GLOBAL _returnfar

[CPU 386]

[BITS 32]

_HALT:
	HLT
RET

_InterruptEnable:
	STI
RET

_InterruptDisable:
	CLI
RET

_InterruptDTabLoad:
	; - [EBP+4*0]=EBX
	; - [EBP+4*1]=Return Address
	; - [EBP+4*2]=Address of IDT
	CLI
	PUSH EBX
	XCHG ESP, EBP
	MOV  EBX, [EBP+4*2]
	LIDT [EBX]
	XCHG ESP, EBP
	POP  EBX
RET

_getCR3:
	MOV EAX, CR3
RET

_getEflags:
	PUSHFD
	POP EAX
RET

_jmpFar:; + TSS_ID_inGDT
	; - [EBP+4*0]=BP
	; - [EBP+4*1]=Return Address
	; - [EBP+4*2]=Address
	; - [EBP+4*3]=Segment(word-eff, dword-len)
	PUSH EBP
	MOV EBP, ESP
	JMP FAR [EBP+4*2]
	MOV ESP, EBP
	POP EBP
RET

_CallFar:
	PUSH EBP
	MOV EBP, ESP
	CALL FAR [EBP+4*2]
	MOV ESP, EBP
	POP EBP
RET

_returnfar:
	RETF
RET
