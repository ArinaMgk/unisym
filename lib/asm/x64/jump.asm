; ASCII NASM TAB4 CRLF
; Docutitle: (Module) Non-local Jumping
; Attribute: Arn-Covenant Arch(x64) Env-Freestanding Non-Dependence
; Copyright: UNISYM, under Apache License 2.0

[BITS 64]
[SECTION .code progbits alloc exec]

GLOBAL MarkPoint
GLOBAL JumpPoint
GLOBAL MarkPointMS
GLOBAL JumpPointMS

; System V AMD64 ABI 3.2.1: Retpoint* buf is passed in RDI.
MarkPoint:
	MOV RAX, [RSP]
	MOV [RDI+ 0], RAX
	MOV [RDI+ 8], RBX
	MOV [RDI+16], RBP
	LEA RAX, [RSP+8]
	MOV [RDI+24], RAX
	MOV [RDI+32], R12
	MOV [RDI+40], R13
	MOV [RDI+48], R14
	MOV [RDI+56], R15
	XOR EAX, EAX
	RET

; System V AMD64 ABI 3.2.1: buf is passed in RDI and val is passed in RSI.
JumpPoint:
	MOV RDX, RDI
	MOV RAX, RSI
	TEST RAX, RAX
	JNZ .value_ready
	MOV EAX, 1
.value_ready:
	MOV RBX, [RDX+ 8]
	MOV RBP, [RDX+16]
	MOV RSP, [RDX+24]
	MOV R12, [RDX+32]
	MOV R13, [RDX+40]
	MOV R14, [RDX+48]
	MOV R15, [RDX+56]
	JMP QWORD [RDX+0]

; Microsoft x64 ABI for the current -mno-sse target: RetpointMS* buf is passed in RCX.
; XMM6-XMM15 are omitted because this target does not generate SSE instructions.
MarkPointMS:
	MOV RAX, [RSP]
	MOV [RCX+ 0], RAX
	MOV [RCX+ 8], RBX
	MOV [RCX+16], RBP
	MOV [RCX+24], RDI
	MOV [RCX+32], RSI
	LEA RAX, [RSP+8]
	MOV [RCX+40], RAX
	MOV [RCX+48], R12
	MOV [RCX+56], R13
	MOV [RCX+64], R14
	MOV [RCX+72], R15
	XOR EAX, EAX
	RET

; Microsoft x64 ABI: buf is passed in RCX and val is passed in RDX.
JumpPointMS:
	MOV R8, RCX
	MOV RAX, RDX
	TEST RAX, RAX
	JNZ .ms_value_ready
	MOV EAX, 1
.ms_value_ready:
	MOV RBX, [R8+ 8]
	MOV RBP, [R8+16]
	MOV RDI, [R8+24]
	MOV RSI, [R8+32]
	MOV RSP, [R8+40]
	MOV R12, [R8+48]
	MOV R13, [R8+56]
	MOV R14, [R8+64]
	MOV R15, [R8+72]
	JMP QWORD [R8+0]
