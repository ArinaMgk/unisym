; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86)
; AllAuthor: @ArinaMgk
; ModuTitle: Interrupt
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

[BITS 32]

%macro DefExc 1
	POP  EAX
	PUSH EBP
	MOV  EBP, ESP
	PUSHAD
	PUSH EAX
	PUSH DWORD %1
	JMP  ERQ_Handler
%endmacro
%macro DefExc_nopara 1
	PUSH EBP
	MOV  EBP, ESP
	PUSHAD
	PUSH DWORD 0
	PUSH DWORD ~%1
	JMP  ERQ_Handler
%endmacro

%ifdef _MCCA
%if _MCCA==0x8632
EXTERN exception_handler
GLOBAL PG_PUSH, PG_POP; PUSH 6*DWORD

ROOT_PAGING EQU 0x00100000
SegData EQU 8*3

; should keep user stack for R0 constant
;{TEMP} 2-lev paging
%macro ConvertStackPointer 0; (ECX:ESP, EDX:CR3)->ESP
	; PUSH EBX
	MOV EBX, ECX
	SHR EBX, 22; 22 for L1P_ID
	MOV EAX, [EDX + EBX * 4]
	TEST EAX, 0x80; huge page 4MB
	JZ  %%L1P
	AND EAX, 0xFFFFF000
	MOV EBX, ECX
	AND EBX, 0x3FFFFF
	OR  EAX, EBX
	OR  EAX, 0x8000_0000; MCCA Design
	JMP %%ENDO
	;
	%%L1P:
	AND EAX, 0xFFFFF000
	MOV EBX, ECX
	SHR EBX, 12; 12 for L0P_ID
	AND EBX, 0x3FF
	MOV EAX, [EAX + EBX * 4]
	%%L0P:
	AND EAX, 0xFFFFF000
	MOV EBX, ECX
	AND EBX, 0xFFF
	OR  EAX, EBX
	OR  EAX, 0x8000_0000; MCCA Design
	%%ENDO:
	; ADD EAX, 4; Skip Ret-address
	; POP EBX
	; RET
	; no use kernel stack
%endmacro

PG_PUSH:
	POP ESI
	MOV EAX, DS
	PUSH EAX
	MOV EAX, SS
	PUSH EAX
	;
	MOV EAX, SegData
	MOV DS, EAX
	MOV ES, EAX
	MOV FS, EAX
	MOV GS, EAX
	MOV SS, EAX
	;
	MOV EDX, CR3
	MOV ECX, ESP
	SUB ECX, 4 * 4; ecx and edx and ebp
	PUSH EDX
	PUSH ECX
		PUSH EBP
	PUSH EBX
	MOV EAX, ROOT_PAGING
	MOV CR3, EAX
	;MOV ESP, 0x7000; INTERRUPT STACK
	ConvertStackPointer
	MOV ESP, EAX
		MOV ECX, EBP
		ConvertStackPointer
		MOV EBP, EAX
	POP  EBX
	PUSH ESI
	RET
PG_POP:
	POP ESI
	    POP EBP
	POP ECX
	POP EDX
	ADD ECX, 4 * 4
	MOV CR3, EDX
	MOV ESP, ECX
	POP EAX
	MOV SS, EAX
	POP EAX
	MOV DS, EAX
	MOV ES, EAX
	MOV FS, EAX
	MOV GS, EAX
	PUSH ESI
	RET

ERQ_Handler:
	POP  EBX
	POP  EDI
	CALL PG_PUSH
	PUSH EDI
	PUSH EBX
	CALL exception_handler
	ADD  ESP, 4*2
	CALL PG_POP
	POPAD
	LEAVE
IRET

GLOBAL Divide_By_Zero_ERQHandler; 0x00
Divide_By_Zero_ERQHandler:
	DefExc_nopara 0x00

GLOBAL Step_ERQHandler; 0x01
Step_ERQHandler:
	DefExc_nopara 0x01

GLOBAL NMI_ERQHandler; 0x02
NMI_ERQHandler:
	CALL PG_PUSH
	PUSH DWORD ~0x02
	CALL exception_handler
HLT


GLOBAL Breakpoint_ERQHandler; 0x03
Breakpoint_ERQHandler:
	DefExc_nopara 0x03

GLOBAL Overflow_ERQHandler; 0x04
Overflow_ERQHandler:
	DefExc_nopara 0x04

GLOBAL Bound_ERQHandler; 0x05
Bound_ERQHandler:
	DefExc_nopara 0x05

GLOBAL Invalid_Opcode_ERQHandler; 0x06
Invalid_Opcode_ERQHandler:
	PUSHAD
	CALL PG_PUSH
	PUSH DWORD ~0x06
	CALL exception_handler
	POP  EAX
	CALL PG_POP
	POPAD
	ADD  DWORD[ESP], 2; length of UD
IRET

GLOBAL Coprocessor_Not_Available_ERQHandler; 0x07
Coprocessor_Not_Available_ERQHandler:
	DefExc_nopara 0x07

GLOBAL Double_Fault_ERQHandler; 0x08
Double_Fault_ERQHandler:
	DefExc 0x08

GLOBAL Coprocessor_Segment_Overrun_ERQHandler; 0x09
Coprocessor_Segment_Overrun_ERQHandler:
	DefExc_nopara 0x09

GLOBAL Invalid_TSS_ERQHandler; 0x0A
Invalid_TSS_ERQHandler:
	DefExc 0x0A

GLOBAL x0B_ERQHandler; 0x0B
x0B_ERQHandler:
	DefExc 0x0B

GLOBAL x0C_ERQHandler; 0x0C
x0C_ERQHandler:
	DefExc 0x0C

GLOBAL x0D_ERQHandler; 0x0D
x0D_ERQHandler:
	DefExc 0x0D

GLOBAL Page_Fault_ERQHandler; 0x0E
Page_Fault_ERQHandler:
; CR2 is virtual address
	POP  EAX
	PUSH EBP
	MOV  EBP, ESP
	PUSHAD
	PUSH EAX
	PUSH DWORD 0x0E
	POP  EBX
	POP  EDI
	CALL PG_PUSH
	PUSH EDI
	PUSH EBX
	MOV  EDI, CR3
	CALL exception_handler
	ADD  ESP, 4*2
	CALL PG_POP
	POPAD
	LEAVE
IRET

GLOBAL x0F_ERQHandler; 0x0F
x0F_ERQHandler:
	DefExc_nopara 0x0F

GLOBAL X87_FPU_Floating_Point_Error_ERQHandler; 0x10
X87_FPU_Floating_Point_Error_ERQHandler:
	DefExc_nopara 0x10

GLOBAL Alignment_Check_ERQHandler; 0x11
Alignment_Check_ERQHandler:
	DefExc 0x11

GLOBAL Machine_Check_ERQHandler; 0x12
Machine_Check_ERQHandler:
	DefExc_nopara 0x12

GLOBAL SIMD_Floating_Point_Exception_ERQHandler; 0x13
SIMD_Floating_Point_Exception_ERQHandler:
	DefExc_nopara 0x13

GLOBAL Virtualization_Exception_ERQHandler; 0x14
Virtualization_Exception_ERQHandler:
	DefExc_nopara 0x14

;

GLOBAL Else_ERQHandler
Else_ERQHandler:
	DefExc_nopara 0x20


%endif
%endif
