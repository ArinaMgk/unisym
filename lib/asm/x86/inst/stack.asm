; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86)
; LastCheck: 2024Mar05
; AllAuthor: @dosconio
; ModuTitle: Interrupt
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

GLOBAL _pushad ; checked: none
GLOBAL _pushfd ; checked: none
GLOBAL _popad  ; checked: none
GLOBAL _popfd  ; checked: none

[CPU 386]

[BITS 16]
	;{}...
	RET

[BITS 32]
; Dosconio's borrowing method
_pushad:; do not change flags
	PUSHAD
	MOV EBP, ESP
	MOV EBX, EBP; EDI
	MOV ECX, 8
	MOV EDX, [EBX]
	PUSHFD
	ADD EBX, 4
	_pushad_loop:
		MOV EAX, [EBX]
		MOV [EBX], EDX
		ADD EBX, 4
		MOV EDX, EAX
		LOOP _pushad_loop
	MOV [EBP], EDX; return address
	; NOW EBP, ESP
	; ESP+0*4 return address
	; ESP+1*4 EDI
	; ESP+2*4 ESI
	; ESP+3*4 EBP
	; ESP+4*4 ESP
	; ESP+5*4 EBX
	; ESP+6*4 EDX
	; ESP+7*4 ECX
	; ESP+8*4 EAX
	MOV EAX, [EBP+4*8]
	MOV ECX, [EBP+4*7]
	MOV EDX, [EBP+4*6]
	MOV EBX, [EBP+4*5]
	ADD DWORD[EBP+4*4], 4; skip return address
	MOV EBP, [EBP+4*3]
	POPFD
	RET
_pushfd:; do not change flags
	PUSHFD
	PUSH EBP
	MOV EBP, ESP
	; ESP+0*4 EBP
	; ESP+1*4 EFLAG
	; ESP+2*4 return address
	; after:
	; ESP+2*4 EFLAG
	; ESP+1*4 return address
	; ESP+0*4 EBP
	PUSH EAX
	PUSH EDX
	MOV EAX, [EBP+4*2]
	MOV EDX, [EBP+4*0]
	MOV [EBP+4*0], EAX
	MOV [EBP+4*2], EDX
	POP EDX
	POP EAX
	POP EBP
	RET

_popad:; do not change flags
	; POPAD(cannot use this)
	; ESP+0*4 return address
	; ESP+1*4 EDI
	; ESP+2*4 ESI
	; ESP+3*4 EBP <last>
	; ESP+4*4 ESP
	; ESP+5*4 EBX
	; ESP+6*4 EDX
	; ESP+7*4 ECX
	; ESP+8*4 EAX
	MOV EBP, ESP
	MOV EDI, [EBP+4*1]
	MOV ESI, [EBP+4*2]
	MOV ESP, [EBP+4*4]
	;
	MOV EAX, [EBP+4*8] 
	PUSH DWORD[EBP]
	MOV EBX, [EBP+4*5]
	MOV EDX, [EBP+4*6]
	MOV ECX, [EBP+4*7]
	MOV EBP, [EBP+4*3]
	RET

_popfd:
	; ESP+0*4 EBP
	; ESP+1*4 return address
	; ESP+2*4 EFLAG
	PUSH EBP
	MOV EBP, ESP
	PUSH EAX
	PUSH EDX
	MOV EAX, [EBP+4*2]
	MOV EDX, [EBP+4*1]
	MOV [EBP+4*1], EAX
	MOV [EBP+4*2], EDX
	POP EDX
	POP EAX
	POP EBP
	POPFD
	RET
