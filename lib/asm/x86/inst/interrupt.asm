; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86)
; LastCheck: 2024Mar05
; AllAuthor: @dosconio
; ModuTitle: Interrupt
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

GLOBAL _returni

[CPU 386]

[BITS 16]
	;{}...
	RET

[BITS 32]

_returni:
	MOV ESP, EBP
	POP EBP
	IRET
