; ASCII N/AASM TAB4 CRLF
; Attribute: ArnCovenant Free bin^32 CPU(80586+)
; LastCheck: 2023 Nov 16
; AllAuthor: @dosconio
; ModuTitle: Instruction CPUID
; Copyright: ArinaMgk UniSym, Apache License Version 2.0
[CPU 586]
%include "../../../Kasha/n_cpuins.a"

[SECTION .data]
	DB "UNISYM DOSCON.IO", "CPUID"

[SECTION .code]
	GLOBAL @CpuBrand@4
	[BITS 32]

; ____________________________________________________________________________

; Get CPU Brand Informatiom
; [__fastcall] CpuBrand <<< ECX(char* BufAddr)
@CpuBrand@4:
	PUSHAD
	MOV ESI, ECX
	CpuBrand
	POPAD
RET
