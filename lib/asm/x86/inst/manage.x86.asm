; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86) Format(ELF)
; LastCheck: 2024Feb18
; AllAuthor: @dosconio
; ModuTitle: 
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

GLOBAL _HALT
GLOBAL _InterruptEnable, _InterruptDisable, _InterruptDTabLoad

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
