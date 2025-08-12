; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86) Format(ELF) Call(not fastcall but cdecl)
; LastCheck: 2024Feb18
; AllAuthor: @dosconio
; ModuTitle: Input and Output for the Port
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

;{TODO} add `x86` in document name;

GLOBAL _OUT_b, _IN_b; out dx, al; in al, dx;
GLOBAL OUT_b, IN_b;
GLOBAL _OUT_w, _IN_w; out dx, ax; in ax, dx;
GLOBAL OUT_w, IN_w;
GLOBAL IN_wn, _IN_wn;
GLOBAL OUT_wn, _OUT_wn;

[CPU 386]

[BITS 32]

; __fastcall void OUT_b(word Port, word Data);
; VOID <<< _OUT_b <<< ECX(DX Port), EDX(AX Data)
_OUT_b:
OUT_b:
;	XCHG CX, DX
;	XCHG CX, AX
;	OUT DX, AL
;	XCHG CX, AX
;	XCHG CX, DX
	;{TEMP} Linux did not accept __fastcall, so use CDECL
	; - [EBP+4*0]=BP
	; - [EBP+4*1]=Return Address
	; - [EBP+4*2]=Port
	; - [EBP+4*3]=Data
	PUSH EBP
	MOV EBP, ESP
	PUSH DX
	PUSH AX
	MOV DX, [EBP+4*2]
	MOV AX, [EBP+4*3]
	OUT DX, AL
	POP AX
	POP DX
	MOV ESP, EBP
	POP EBP
RET

; __fastcall word IN_b(word Port);
; BYTE <<< _IN_b <<< ECX(Port)
_IN_b:
IN_b:
;	XCHG CX, DX
;	IN AL, DX
;	MOVZX EAX, AL
;	XCHG CX, DX
	;{TEMP} Linux did not accept __fastcall, so use CDECL
	; - [EBP+4*0]=BP
	; - [EBP+4*1]=Return Address
	; - [EBP+4*2]=Port
	PUSH EBP
	MOV EBP, ESP
	PUSH DX
	MOV DX, [EBP+4*2]
	IN AL, DX
	MOVZX EAX, AL
	POP DX
	MOV ESP, EBP
	POP EBP
RET

; __fastcall void OUT_w(word Port, word Data);
; VOID <<< _OUT_w <<< ECX(Port), EDX(Data)
_OUT_w:
OUT_w:
;	XCHG CX, DX
;	XCHG CX, AX
;	OUT DX, AX
;	XCHG CX, AX
;	XCHG CX, DX
	;{TEMP} Linux did not accept __fastcall, so use CDECL
	; - [EBP+4*0]=BP
	; - [EBP+4*1]=Return Address
	; - [EBP+4*2]=Port
	; - [EBP+4*3]=Data
	PUSH EBP
	MOV EBP, ESP
	PUSH DX
	PUSH AX
	MOV DX, [EBP+4*2]
	MOV AX, [EBP+4*3]
	OUT DX, AX
	POP AX
	POP DX
	MOV ESP, EBP
	POP EBP
RET

; __fastcall word IN_w(word Port);
; WORD <<< _IN_w <<< ECX(Port)
_IN_w:
IN_w:
;	XCHG CX, DX
;	IN AX, DX
;	XCHG CX, DX
	;{TEMP} Linux did not accept __fastcall, so use CDECL
	; - [EBP+4*0]=BP
	; - [EBP+4*1]=Return Address
	; - [EBP+4*2]=Port
	PUSH EBP
	MOV EBP, ESP
	PUSH DX
	MOV DX, [EBP+4*2]
	IN AX, DX
	POP DX
	MOV ESP, EBP
	POP EBP
RET

IN_wn:
_IN_wn:
; void IN_wn(word Port, word* Data, unsigned n);
	MOV EDX, [ESP + 4]
	MOV EDI, [ESP + 8]
	MOV ECX, [ESP + 12]
	SHR ECX, 1
	CLD
	REP INSW
	RET

OUT_wn:
_OUT_wn:
; void OUT_wn(word Port, word* Data, unsigned n);
	MOV EDX, [ESP + 4]
	MOV ESI, [ESP + 8]
	MOV ECX, [ESP + 12]
	SHR ECX, 1
	CLD
	REP OUTSW
	RET

