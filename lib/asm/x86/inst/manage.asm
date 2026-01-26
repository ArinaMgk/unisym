; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86) Format(ELF)
; LastCheck: 2024Feb18
; AllAuthor: @dosconio
; ModuTitle: 
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

GLOBAL _HALT, HALT
GLOBAL _InterruptDisable, InterruptDTabLoad
GLOBAL  InterruptEnable,  InterruptDisable
GLOBAL getCR0,getCR2,getCR3
GLOBAL getFlags
GLOBAL setA

GLOBAL jmpFar, CallFar
GLOBAL _returnfar
GLOBAL returnfar
GLOBAL TaskReturn

GLOBAL IfSupport_IA32E

[CPU 386]

[BITS 32]


section .text

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

getCR0:
	MOV EAX, CR0
; getCR1:
; 	MOV EAX, CR1
getCR2:
	MOV EAX, CR2
getCR3:
	MOV EAX, CR3
RET

getFlags:
	PUSHFD
	POP EAX
RET

setA:
	; - [EBP+4*0]=EBP
	; - [EBP+4*1]=Return Address
	; - [EBP+4*2]=Parameter
	PUSH EBP
	MOV EBP, ESP
	MOV EAX, [EBP+8]
	POP EBP
RET

; ---- ---- JMPs ---- ---- ;

;struct __attribute__((packed)) tmp48be_t { uint32 u_32fore; uint16 u_16back; };
;VER 1 may break the stack
;VER 2 since 20250122
jmpFar:; + TSS_ID_inGDT
	POP EAX; noreturn
RETF; eat 2 dwords

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
