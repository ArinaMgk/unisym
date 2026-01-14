; ASCII NASM TAB4 CRLF
; Attribute: CPU(x64) Call(System V AMD64)
; AllAuthor: @ArinaMgk
; ModuTitle: Input and Output for the Port
; Copyright: ArinaMgk UniSym, Apache License Version 2.0
; RegisterI: RDI, RSI, RDX, RCX, R8, R9
; RegisterO: RAX

GLOBAL OUT_b, IN_b;
GLOBAL OUT_w, IN_w;
GLOBAL OUT_d, IN_d;

[BITS 64]

OUT_b:
	MOV DX, DI    ;  DX = PORT
	MOV AX, SI    ; EAX = DATA
	OUT DX, AL
RET

IN_b:
	MOV DX, DI    ;  DX = PORT
	IN  AL, DX
RET

OUT_w:
	MOV DX, DI    ;  DX = PORT
	MOV AX, SI    ; EAX = DATA
	OUT DX, AX
RET

IN_w:
	MOV DX, DI    ;  DX = PORT
	IN  AX, DX
RET

; (u16 port, u32 data);
; VOID <<< OUT_d <<< RDI(Port), RSI(AX Data)
OUT_d:
	MOV DX, DI    ;  DX = PORT
	MOV EAX, ESI  ; EAX = DATA
	OUT DX, EAX
RET

; (u16 addr) -> u32;
; RAX <<< IN_d <<< RDI(Port)
IN_d:
	MOV DX, DI    ;  DX = PORT
	IN  EAX, DX
RET


