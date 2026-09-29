; ASCII [NASM & AASM] Binary32 Intel80386+
; x86-call-convetion arina-covenant
; Copyright (C) 2023 unisym of Arinae, open source under the Apache License 2.0
; Author: @dosconio @ArinaMgk
[BITS 32]
[CPU 386]

[SECTION .data]
	DB "UNISYM DOSCON.IO"

[SECTION .code]
	%ifndef _DYNLINK_
	GLOBAL @StrLength@4
	%endif
	GLOBAL StrLength
; ____________________________________________________________________________

; EAX <<< _StrLength <<< ECX(const char*)
; Windows __fastcall
; Get the length of a ASCIZ string
; {TODO: optimize with SCASD and check 4 bytes for each iteration}
; - "\0" ---> FFFFFFFF
StrLength:
%ifndef _DYNLINK_
@StrLength@4:
%endif
	PUSH EDI
	PUSH ECX
	MOV EDI, ECX
	XOR EAX, EAX
	XOR ECX, ECX
	DEC ECX; i.e. NOT ECX
	CLD
	REPNZ SCASB
	MOV EAX, ECX
	NOT EAX
	DEC EAX
	POP ECX
	POP EDI
RET

;StrIndex
	; REPNZ SCASB
	; JZ <found>
