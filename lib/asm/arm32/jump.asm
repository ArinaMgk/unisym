; ASCII AASM TAB4 LF
; Cortex-M7 non-local jump support.

[ARCH arm32]
[CORE cortex-m7]
[INST T32]
[BITS 32]
[SECTION .text]

GLOBAL MarkPoint:function
GLOBAL JumpPoint:function

;{TODO} use __ARCH__ and other built-in macros to use below code
; %if

; ARM procedure-call standard: Retpoint* is passed in R0.
MarkPoint:
	MOV R1, LR
	STR R1, [R0 + 0]
	MOV R1, SP
	STR R1, [R0 + 4]
	STR R4, [R0 + 8]
	STR R5, [R0 + 12]
	STR R6, [R0 + 16]
	STR R7, [R0 + 20]
	STR R8, [R0 + 24]
	STR R9, [R0 + 28]
	STR R10, [R0 + 32]
	STR R11, [R0 + 36]
	MOVS R0, 0
	BX LR

; Retpoint* and value are passed in R0 and R1 respectively.
JumpPoint:
	MOV R2, R0
	MOV R3, R1
	CMP R3, 0
	BNE .value_ready
	MOVS R3, 1
.value_ready:
	LDR R4, [R2 + 8]
	LDR R5, [R2 + 12]
	LDR R6, [R2 + 16]
	LDR R7, [R2 + 20]
	LDR R8, [R2 + 24]
	LDR R9, [R2 + 28]
	LDR R10, [R2 + 32]
	LDR R11, [R2 + 36]
	LDR R12, [R2 + 0]
	LDR R1, [R2 + 4]
	MOV SP, R1
	MOV R0, R3
	BX R12

; %endif
