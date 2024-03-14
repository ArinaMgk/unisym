; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86)
; LastCheck: 2024Feb16
; AllAuthor: @dosconio
; ModuTitle: Memory Media - Floppy 
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

GLOBAL FloppyMotorOff

[CPU 386]

[BITS 16]

FloppyMotorOff:
	PUSH AX
	PUSH DX
	MOV  DX, 0x3F2
	MOV  AL, 0; 0x0C?
	OUT  DX, AL
	POP  DX
	POP  AX
	RET
