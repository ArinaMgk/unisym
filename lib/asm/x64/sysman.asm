; ASCII NASM TAB4 CRLF
; Attribute: CPU(x64)
; Codifiers: @ArinaMgk
; Docutitle: 
; Copyright: ArinaMgk UniSym, Apache License Version 2.0
GLOBAL EnableSSE
GLOBAL IfSupport_IA32E

[BITS 64]

EnableSSE:
; EM (Emulation)
; MP (Monitor Coprocessor)
	CLTS; clear CR0.TS(3)
	MOV RAX, CR0
	BTR RAX, 1; MP(1)
	BTR RAX, 2; EM(2)
	MOV CR0, RAX
	MOV RAX, CR4
	BTS RAX, 9; OSFXSR
	BTS RAX, 10; OSXMMEXCPT
	MOV CR4, RAX
RET
