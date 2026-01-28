; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86)
; Codifiers: @ArinaMgk
; Docutitle: 
; Copyright: ArinaMgk UniSym, Apache License Version 2.0
GLOBAL EnableSSE
GLOBAL IfSupport_IA32E

[BITS 32]

EnableSSE:
; EM (Emulation)
; MP (Monitor Coprocessor)
	CLTS; clear CR0.TS
	MOV EAX, CR0
	AND EAX, 0xFFFFFFF1; TS(3) EM(2) MP(1)
	MOV CR0, EAX
	MOV EAX, CR4
	BTS EAX, 9; OSFXSR
	BTS EAX, 10; OSXMMEXCPT
	MOV CR4, EAX
RET

[CPU 586]
IfSupport_IA32E:
	PUSH EBX
	PUSH ECX
	PUSH EDX
	MOV  EAX, 0x80000000
	CPUID
	CMP  EAX, 0x80000001
	SETNB AL
	JB   BR_SUPPORT_IA32E
	MOV  EAX, 0x80000001
	CPUID
	BT   EDX, 29
	SETC AL
	BR_SUPPORT_IA32E: MOVZX EAX, AL
	POP  EDX
	POP  ECX
	POP  EBX
RET



