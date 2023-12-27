; ASCII N/AASM TAB4 CRLF
; Attribute: ArnCovenant CPU(80586+)
; LastCheck: RFZ03
; AllAuthor: @dosconio
; ModuTitle: Instruction CPUID
; Copyright: ArinaMgk UniSym, Apache License Version 2.0
[CPU 586]
%include "../../../Kasha/n_cpuins.a"

[SECTION .data]
	DB "UNISYM DOSCON.IO", "CPUID"

[SECTION .code]
	GLOBAL @CpuBrand@4 
	GLOBAL CpuBrand
	[BITS 32]

; ____________________________________________________________________________

; Get CPU Brand Informatiom
; [__fastcall] CpuBrand <<< ECX(char* BufAddr)
@CpuBrand@4:
CpuBrand:
	PUSHAD
	MOV ESI, ECX
	M_CpuBrand
	POPAD
RET
