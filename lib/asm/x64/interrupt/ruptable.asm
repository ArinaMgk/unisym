; ASCII NASM TAB4 CRLF
; Attribute: CPU(x64)
; AllAuthor: @ArinaMgk
; ModuTitle: Interrupt
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

[BITS 64]

%macro DefExc 1
	PUSH %1
	CALL ERQ_Handler
	ADD RSP, 8*2
	HLT
%endmacro
%macro DefExc_nopara 1
	PUSH RAX; store a meaningless value to occupy
	PUSH %1
	CALL ERQ_Handler
	ADD RSP, 8*2
	HLT
%endmacro

%ifdef _MCCA
%if _MCCA==0x8664
EXTERN exception_handler
GLOBAL PG_PUSH, PG_POP; PUSH 6*DWORD

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
OFF_RET EQU 0x88
OFF_FUN EQU 0x90
OFF_ERR EQU 0x98
OFF_RIP EQU 0xA0
OFF_CS  EQU 0xA8
OFF_FLG EQU 0xB0
OFF_RSP EQU 0xB8
OFF_SS  EQU 0xC0

ROOT_PAGING EQU 0xFFFF_FFFFC0000508

; -----------------------------------------------------------------------------
; Helper Function: Convert_VA_to_PA
; Walks the 4-level page table to convert a Virtual Address to a Physical Address.
; Inputs:  RAX = Virtual Address to convert
;          R8  = CR3 (Physical Address of PML4)
;          RSI = Physical-to-Virtual Offset (for memory access)
; Outputs: RAX = Physical Address (which is also the Identity VA in ROOT_PAGING)
; Trashes: R9, RCX, R11
; -----------------------------------------------------------------------------
Convert_VA_to_PA:
	; --- Level 4: PML4 (Bits 39-47) ---
	MOV R9, R8
	MOV R11, 0x000FFFFFFFFFF000
	AND R9, R11 ; Mask out PCID/Flags from CR3
	ADD R9, RSI                ; Convert physical PML4 base to virtual address
	MOV RCX, RAX
	SHR RCX, 39
	AND RCX, 0x1FF             ; Extract PML4 Index
	MOV R9, [R9 + RCX * 8]     ; Read PML4 Entry
	TEST R9, 1                 ; Check Present bit (Bit 0)
	JZ .unmapped

	; --- Level 3: PDPT (Bits 30-38) ---
	AND R9, R11 ; Mask out flags
	ADD R9, RSI                ; Convert physical PDPT base to virtual address
	MOV RCX, RAX
	SHR RCX, 30
	AND RCX, 0x1FF             ; Extract PDPT Index
	MOV R9, [R9 + RCX * 8]     ; Read PDPT Entry
	TEST R9, 1                 ; Check Present bit
	JZ .unmapped
	TEST R9, 0x80              ; Check PS (Page Size) bit (Bit 7)
	JNZ .is_1gb_page           ; If set, it's a 1GB huge page

	; --- Level 2: PD (Bits 21-29) ---
	AND R9, R11 ; Mask out flags
	ADD R9, RSI                
	MOV RCX, RAX
	SHR RCX, 21
	AND RCX, 0x1FF             ; Extract PD Index
	MOV R9, [R9 + RCX * 8]     ; Read PD Entry
	TEST R9, 1
	JZ .unmapped
	TEST R9, 0x80              ; Check PS (Page Size) bit (Bit 7)
	JNZ .is_2mb_page           ; If set, it's a 2MB large page
	; --- Level 1: PT (Bits 12-20) ---
	AND R9, R11 ; Mask out flags
	ADD R9, RSI                
	MOV RCX, RAX
	SHR RCX, 12
	AND RCX, 0x1FF             ; Extract PT Index
	MOV R9, [R9 + RCX * 8]     ; Read PT Entry
	TEST R9, 1
	JZ .unmapped
	.is_4kb_page:
		AND R9, R11 ; Base physical address of 4KB page
		MOV RCX, RAX
		AND RCX, 0xFFF             ; Extract low 12 bits (Offset)
		ADD R9, RCX                ; Add offset to base
		MOV RAX, R9                ; Return physical address
		RET
	.is_2mb_page:
		MOV R11, 0x000FFFFFFFE00000
		AND R9, R11 ; Base physical address of 2MB page (Clear bottom 21 bits)
		MOV RCX, RAX
		AND RCX, 0x1FFFFF          ; Extract low 21 bits (Offset)
		ADD R9, RCX
		MOV RAX, R9
		RET
	.is_1gb_page:
		MOV R11, 0x000FFFFFC0000000
		AND R9, R11 ; Base physical address of 1GB page (Clear bottom 30 bits)
		MOV RCX, RAX
		AND RCX, 0x3FFFFFFF        ; Extract low 30 bits (Offset)
		ADD R9, RCX
		MOV RAX, R9
		RET
	.unmapped:
		; If the current stack is unmapped, the system cannot recover safely.
		UD2                        ; Trigger Invalid Opcode exception (Panic)

PG_PUSH:
	POP  R10
	MOV  RBX, CR3
	MOV  RDX, RSP
	PUSH RBX
	PUSH RDX
	MOV  RDX, ROOT_PAGING
	MOV  RDX, [RDX]
	CMP  RDX, RBX
	JZ   PG_PUSH_ENDO
	MOV  RAX, RSP
	MOV  R8, RBX
	MOV  RSI, 0xFFFF_FFFFC0000000
	CALL Convert_VA_to_PA
	MOV  RSP, RAX
	MOV  CR3, RDX
	PG_PUSH_ENDO: PUSH R10
RET


PG_POP:
	POP R10
	POP RBX; RSP
	POP RAX; CR3
	MOV RCX, CR3
	CMP RCX, RAX
	JZ  PG_POP_ENDO
	MOV CR3, RAX
	PG_POP_ENDO:
	MOV RSP, RBX
	PUSH R10
RET

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
	CALL PG_PUSH
	; CALL: x64 System VCall
	MOV RDI, [RSP + OFF_FUN + 8*2]
	MOV RSI, [RSP + OFF_ERR + 8*2]
	CALL exception_handler
RET_EXCEPTION:
	CALL PG_POP
	POP R15
	POP R14
	POP R13
	POP R12
	POP R11
	POP R10
	POP R9
	POP R8
	POP RBX
	POP RCX
	POP RDX
	POP RSI
	POP RDI
	POP RBP
	POP RAX
	MOV DS, EAX
	POP RAX
	MOV ES, EAX
	POP RAX
RET


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
	PUSH RAX; store a meaningless value to occupy
	PUSH 0x06
	CALL ERQ_Handler
	ADD  RSP, 8*2
	ADD  QWORD[RSP], 2
IRETQ

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
	;PUSH R15
	PUSH 0x0E
	MOV R15, CR3
	CALL ERQ_Handler
	ADD RSP, 8*2
	HLT

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
