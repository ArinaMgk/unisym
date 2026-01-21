; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86)
; AllAuthor: @ArinaMgk
; ModuTitle: Interrupt
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

%macro DefExc 1
	PUSH EBP
	MOV  EBP, ESP
	PUSH EAX
	MOV  EAX, %1
	PUSH EAX
	CALL ERQ_Handler
	POP  EAX
	POP  EAX
	LEAVE
IRET
%endmacro
%macro DefExc_nopara 1
	PUSH EBP
	MOV  EBP, ESP
	PUSH EAX
	MOV  EAX, %1
	NOT  EAX
	PUSH EAX
	CALL ERQ_Handler
	POP  EAX
	POP  EAX
	LEAVE
IRET
%endmacro

%ifdef _MCCA
%if _MCCA==0x8632
EXTERN ERQ_Handler

GLOBAL Divide_By_Zero_ERQHandler; 0x00
Divide_By_Zero_ERQHandler:
	DefExc_nopara 0x00

GLOBAL Step_ERQHandler; 0x01
Step_ERQHandler:
	DefExc_nopara 0x01

GLOBAL NMI_ERQHandler; 0x02
NMI_ERQHandler:
	MOV  EAX, 0x02
	NOT  EAX
	PUSH EAX
	JMP  ERQ_Handler

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
	PUSH EBP
	MOV  EBP, ESP
	PUSHAD
	MOV  EAX, 0x06
	NOT  EAX
	PUSH EAX
	CALL ERQ_Handler
	POP  EAX
	POPAD
	LEAVE
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
