; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86) Format(ELF)
; LastCheck: 2024Feb18
; AllAuthor: @dosconio
; ModuTitle: 
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

GLOBAL _HALT
GLOBAL _InterruptEnable, _InterruptDisable

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
