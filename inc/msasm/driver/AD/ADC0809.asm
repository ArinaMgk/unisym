; ASCII [MASM] TAB4 CRLF
; Attribute: MCU template
; LastCheck: RFX29
; AllAuthor: @dosconio
; ModuTitle: Intel Chip 8259A
; Copyright: ArinaMgk UniSym, Apache License Version 2.0


ADC0809_Send macro port_adc
	push dx
	mov dx, port_adc
	mov al, 0
	out dx, al
	pop dx
endm

ADC0809_Receive macro port_adc
	push dx
	mov dx, port_adc
	; you may need delay
	in al, dx
	pop dx
endm
