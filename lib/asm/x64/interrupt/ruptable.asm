; ASCII NASM TAB4 CRLF
; Attribute: CPU(x64)
; AllAuthor: @ArinaMgk
; ModuTitle: Interrupt
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

[BITS 64]

%macro DefExc 1
	PUSH %1
	JMP ERQ_Handler
%endmacro
%macro DefExc_nopara 1
	PUSH RAX; store a meaningless value to occupy
	PUSH %1
	JMP ERQ_Handler
%endmacro

%ifdef _MCCA
%if _MCCA==0x8664
EXTERN exception_handler
; EXTERN PG_PUSH, PG_POP; PUSH 6*DWORD

OFF_R15 EQU 0x00
OFF_R14 EQU 0x08
OFF_R13 EQU 0x10
OFF_R12 EQU 0x18
OFF_R11 EQU 0x20
OFF_R10 EQU 0x28
OFF_R9  EQU 0x30
OFF_R8  EQU 0x38
OFF_RBX EQU 0x40
OFF_RCX EQU 0x48
OFF_RDX EQU 0x50
OFF_RSI EQU 0x58
OFF_RDI EQU 0x60
OFF_RBP EQU 0x68
OFF_DS  EQU 0x70
OFF_ES  EQU 0x78
OFF_RAX EQU 0x80
;
OFF_FUN EQU 0x88
OFF_ERR EQU 0x90
OFF_RIP EQU 0x98
OFF_CS  EQU 0xA0
OFF_FLG EQU 0xA8
OFF_RSP EQU 0xB0
OFF_SS  EQU 0xB8

ERQ_Handler:
	PUSH RAX
	MOV EAX, ES
	PUSH RAX
	MOV EAX, DS
	PUSH RAX
	PUSH RBP
	PUSH RDI
	PUSH RSI
	PUSH RDX
	PUSH RCX
	PUSH RBX
	PUSH R8
	PUSH R9
	PUSH R10
	PUSH R11
	PUSH R12
	PUSH R13
	PUSH R14
	PUSH R15
	CLD
	;{} PG_PUSH
	; CALL: x64 System VCall
	MOV RDI, [RSP + OFF_FUN]
	MOV RSI, [RSP + OFF_ERR]
	CALL exception_handler
	;{unchk} RET from EXC
HLT

IRET

GLOBAL Divide_By_Zero_ERQHandler; 0x00
Divide_By_Zero_ERQHandler:
	DefExc_nopara 0x00

GLOBAL Step_ERQHandler; 0x01
Step_ERQHandler:
	DefExc_nopara 0x01

GLOBAL NMI_ERQHandler; 0x02
NMI_ERQHandler:
	; CALL PG_PUSH
	MOV  EAX, 0x02
	NOT  RAX
	PUSH RAX
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
	DefExc_nopara 0x06

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
	DefExc 0x0E

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
