; ASCII [MASM] TAB4 CRLF
; Attribute: MCU template
; LastCheck: RFX28
; AllAuthor: @dosconio
; ModuTitle: Intel Chip 8255A
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

; Mode Control Word
I8255A_A_0 EQU 10000000B; Default
I8255A_A_1 EQU 10100000B
I8255A_A_2 EQU 11000000B; 11x?????B

I8255A_A_OUT EQU 10000000B; Default
I8255A_A_IN  EQU 10010000B

I8255A_B_0 EQU 10000000B; Default
I8255A_B_1 EQU 10000100B

I8255A_B_OUT EQU 10000000B; Default
I8255A_B_IN  EQU 10000010B

I8255A_C_OUT  EQU 10000000B; Default
I8255A_C_IN   EQU 10001001B
I8255A_CH_OUT EQU 10000000B; Default
I8255A_CH_IN  EQU 10001000B
I8255A_CL_OUT EQU 10000000B; Default
I8255A_CL_IN  EQU 10000001B

; ---- ---- Frequent ---- ----
I8255A_OUT_ALL EQU 80H; Default


; [0 0 0 0 Idx2 Idx1 Idx0 BinDig]
; DX <<< I8255A_C_Struct <<< DX
; - I8255A_C_Struct macro dig, bit; for MASM5
I8255A_C_Struct macro dig:<0>, bit:<0>
	push dx
	mov dl, dig
	mov dh, bit
	shl dl, 1
	or dl, dh
	pop dx
endm
