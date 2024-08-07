; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86) Bits(32)
; LastCheck: 20240805
; AllAuthor: @dosconio
; ModuTitle: 
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

GLOBAL _setA32

[CPU 386]

[BITS 32]

_setA32:
	; - [EBP+4*0]=EBP
	; - [EBP+4*1]=Return Address
	; - [EBP+4*2]=Parameter
	PUSH EBP
	MOV EBP, ESP
	MOV EAX, [EBP+8]
	POP EBP
RET