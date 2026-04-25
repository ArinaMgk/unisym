; ASCII NASM TAB4 CRLF
; Attribute: CPU(x64)
; Codifiers: @ArinaMgk
; Docutitle: 
; Copyright: ArinaMgk UniSym, Apache License Version 2.0
GLOBAL EnableSSE
GLOBAL IfSupport_IA32E

[BITS 64]

EnableSSE:
; EM (Emulation)
; MP (Monitor Coprocessor)
	CLTS; clear CR0.TS(3)
	MOV RAX, CR0
	BTR RAX, 1; MP(1)
	BTR RAX, 2; EM(2)
	MOV CR0, RAX
	MOV RAX, CR4
	BTS RAX, 9; OSFXSR
	BTS RAX, 10; OSXMMEXCPT
	MOV CR4, RAX
RET

EnableAVX:; Call EnableSSE before this
; OSXSAVE (Bit 18)
	; Enable XSAVE and processor extended states
	MOV RAX, CR4
	BTS RAX, 18		; Set OSXSAVE bit
	MOV CR4, RAX
	; XCR0 (Extended Control Register 0)
	; Configure feature mask for XSAVE/XRSTOR
	XOR ECX, ECX		; Select XCR0 (ECX = 0)
	XGETBV			; Read XCR0 into EDX:EAX
	BTS RAX, 0		; x87 state (Bit 0)
	BTS RAX, 1		; SSE state (Bit 1)
	BTS RAX, 2		; AVX state (Bit 2)
	XSETBV			; Write EDX:EAX back to XCR0
RET
