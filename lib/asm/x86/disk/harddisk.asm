; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86)
; LastCheck: 20250106
; AllAuthor: @dosconio
; ModuTitle: Memory Media - Hard Disk
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

%include "hdisk.a"

GLOBAL HdiskLBA28Load

[CPU 386]

[BITS 32]

HdiskLBA28Load:
	HdiskLoadLBA28 EAX, CL
	RET
