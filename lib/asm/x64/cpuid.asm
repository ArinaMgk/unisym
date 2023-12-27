; ASCII N/AASM(N2.7) TAB4 CRLF
; Attribute: ArnCovenant CPU(x64+)
; LastCheck: RFZ03
; AllAuthor: @dosconio
; ModuTitle: Instruction CPUID
; Copyright: ArinaMgk UniSym, Apache License Version 2.0
%include "../../../Kasha/n_cpuins.a"

[SECTION .data]
	DB "UNISYM DOSCON.IO", "CPUID"

[SECTION .code]
	GLOBAL @CpuBrand@4 
	GLOBAL CpuBrand
	[BITS 64]

; ____________________________________________________________________________

; Get CPU Brand Informatiom
; [__fastcall] CpuBrand <<< ECX(char* BufAddr)
@CpuBrand@4:
CpuBrand:
	PUSH RSI
	PUSH RDI
	PUSH RAX
	PUSH RCX
	PUSH RDX
	PUSH RBX
	MOV RSI, RCX
	; M_CpuBrand
		MOV RDI,0X80000002
		lup:
		MOV RAX, RDI
		INC RDI
		CPUID
		MOV [RSI + 0X00], EAX
		MOV [RSI + 0X04], EBX
		MOV [RSI + 0X08], ECX
		MOV [RSI + 0X0C], EDX
		ADD RSI, 0X10
		CMP EDI, 0X80000004
		JBE lup
	POP RBX
	POP RDX
	POP RCX
	POP RAX
	POP RDI
	POP RSI
RET
