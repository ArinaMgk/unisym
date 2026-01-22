; ASCII NASM TAB4 CRLF
; Attribute: CPU(x64) Format(ELF)
; AllAuthor: @ArinaMgk
; ModuTitle: 
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

GLOBAL HALT
GLOBAL InterruptEnable, InterruptDisable
GLOBAL InterruptDTabLoad
GLOBAL getCR3, getFlags
GLOBAL getCS
GLOBAL setA
GLOBAL jmpFar;, CallFar
; GLOBAL returnfar
; GLOBAL TaskReturn

; [CPU 386]

[BITS 64]


section .text

HALT:
	HLT
RET

InterruptEnable:
	STI
RET

InterruptDisable:
	CLI
RET

InterruptDTabLoad:
	LIDT [RDI]
RET

; ---- ---- G/SETs (Management Registers) ---- ---- ;

getCR3:
	MOV RAX, CR3
RET

getFlags:
	PUSHFQ
	POP RAX
RET

getCS:
	XOR EAX, EAX; Higher 32 bits clear when modifying a 32-bit register in long mode
	MOV AX, CS
RET

setA:
	MOV RAX, RDI
RET

; ---- ---- JMPs ---- ---- ;
; void jmpFar(addr RDI, seg RSI);
jmpFar:
	ADD RSP, 0x8
	MOVZX RSI, SI
	PUSH RSI
	PUSH RDI
O64 RETF

