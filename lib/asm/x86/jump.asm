; ASCII ANY TAB4 CRLF
; Docutitle: (Module) Jumping
; Codifiers: @dosconio: 20240902
; Attribute: Arn-Covenant Arch(x86) Env-Freestanding Non-Dependence
; Copyright: UNISYM, under Apache License 2.0

[bits 32]
[SECTION .code]

GLOBAL _JumpPoint
GLOBAL _MarkPoint

_JumpPoint:; (Retpoint* rp, dword retval)
	MOV EAX, [ESP+ 4]
	MOV EBX, [EAX+20]
	MOV ESI, [EAX+ 0]; `ret`
	MOV [EBX], ESI
	MOV EBX, [EAX+ 4]
	MOV ESI, [EAX+ 8]
	MOV EDI, [EAX+12]
	MOV EBP, [EAX+16]
	MOV EAX, [EAX+20]
	MOV ESP, [ESP+ 8]
	XCHG EAX, ESP
	RET

_MarkPoint:; (Retpoint* rp)
	PUSH EBX
	MOV EBX, [ESP+0+4]
	MOV EAX, [ESP+4+4]
	MOV [EAX], EBX; RET-ADDR
	POP EBX
	MOV [EAX+ 4], EBX
	MOV [EAX+ 8], ESI
	MOV [EAX+12], EDI
	MOV [EAX+16], EBP
	MOV [EAX+20], ESP
	XOR EAX, EAX
	RET

; use aasm and gcc-13.2.0
