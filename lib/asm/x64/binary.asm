; ASCII N/AASM(N2.7) TAB4 CRLF
; Attribute: ArnCovenant Arc(AMD64)
; LastCheck: RFZ04
; AllAuthor: @dosconio
; ModuTitle: Binary Digit Routine
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

[SECTION .data]
	DB "UNISYM DOSCON.IO", "BINARY"

[SECTION .code]
	GLOBAL @BitReflect@8
	GLOBAL BitReflect
	[BITS 64]

; ____________________________________________________________________________

; Reflect D and A, with C as times.
; E.g. Input DL 10101010 (maybe from CPL) and input AL 11110000 (maybe as return for CPL) for CL >= 8
;      prepare: test AL
;      loop 00: RCR to DL(?_1010101), CF=0, RCL to AL(1110000_0), CF=1
;      loop 01: RCR to DL(1?_101010), CF=1, RCL to AL(110000_01), CF=1
;      loop 02: RCR to DL(11?_10101), CF=0, RCL to AL(10000_010), CF=1
;      loop 03: RCR to DL(111?_1010), CF=1, RCL to AL(0000_0101), CF=1
;      loop 04: RCR to DL(1111?_101), CF=0, RCL to AL(000_01010), CF=0
;      loop 05: RCR to DL(01111?_10), CF=1, RCL to AL(00_010101), CF=0
;      loop 06: RCR to DL(001111?_1), CF=0, RCL to AL(0_0101010), CF=0
;      loop 07: RCR to DL(0001111?_), CF=1, RCL to AL(_01010101), CF=0
;      loop 08: RCR to DL(00001111?)
; However, now is for RDX and RAX, with RCX as times.
; [__fastcall] BitReflect8 <<< RCX(times), RDX(inp)
@BitReflect@8:
BitReflect:
	PUSH RCX
	PUSH RDX
	XOR RAX, RAX
	OR RCX, RCX
	JZ BitReflectEndo
	CMP RCX, 64
	JBE BitReflectLoop
	MOV RCX, 64
	BitReflectLoop:
		RCR RDX, 1
		RCL RAX, 1
		LOOP BitReflectLoop
	BitReflectEndo:
	POP RDX
	POP RCX
RET

