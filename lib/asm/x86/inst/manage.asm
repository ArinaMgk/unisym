; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86) Format(ELF)
; LastCheck: 2024Feb18
; AllAuthor: @dosconio
; ModuTitle: 
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

GLOBAL _HALT, HALT
GLOBAL _InterruptDisable, InterruptDTabLoad
GLOBAL  InterruptEnable,  InterruptDisable
GLOBAL getCR3, getEflags
GLOBAL jmpFar, CallFar
GLOBAL _returnfar
GLOBAL returnfar
GLOBAL TaskReturn

[CPU 386]

[BITS 32]

_HALT:
HALT:
	HLT
RET

InterruptEnable:
	STI
RET

_InterruptDisable:
InterruptDisable:
	CLI
RET

InterruptDTabLoad:
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

getCR3:
	MOV EAX, CR3
RET

getEflags:
	PUSHFD
	POP EAX
RET

;struct __attribute__((packed)) tmp48be_t { uint32 u_32fore; uint16 u_16back; };
jmpFar:; + TSS_ID_inGDT
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

CallFar:
	PUSH EBP
	MOV EBP, ESP
	CALL FAR [EBP+4*2]
	MOV ESP, EBP
	POP EBP
RET

_returnfar:
returnfar:
	RETF
RET
ALIGN 16
TaskReturn:
	PUSHFD
	POP EDX
	TEST DX, 0100_0000_0000_0000B
	JNZ NESTED_TASK
	JMP 8*5:0; 8*5 TEMP
RET
	NESTED_TASK: IRETD
RET; for next calling the subapp
ALIGN 16
