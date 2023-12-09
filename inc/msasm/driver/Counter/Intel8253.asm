; ASCII [MASM] TAB4 CRLF
; Attribute: MCU template
; LastCheck: RFX28
; AllAuthor: @dosconio
; ModuTitle: Intel Chip 8253
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

; Offset 8-bit
I8253_CONTROL = 03H
I8253_COUNT0  = 00H
I8253_COUNT1  = 01H                  
I8253_COUNT2  = 02H

;{TODO}::

; Period = clk_a * clk_b * 1e6 second
I8253_Init macro port_imm, clk_a, clk_b
	push ax
	push dx
	; [SEL1 SEL0 RWH RWL Mod2 Mod1 Mod0 BCD] Counter 0, Mode 3, Hexa(NOT BCD)
	mov   al, 00110110B  
	mov   dx, port_imm + I8253_CONTROL
	out   dx, al
	; Initialize Counter 0
	mov   ax, clk_a
	mov   dx, port_imm + I8253_COUNT0
	out   dx, al; Low
	mov   al, ah
	out   dx, al; High
	; Counter 1, Mode 3, Hexa(NOT BCD)
	mov   al, 76H; 01110110B  
	mov   dx, port_imm + I8253_CONTROL
	out   dx, al
	; Initialize Counter 1
	mov   ax, clk_b
	mov   dx, port_imm + I8253_COUNT1
	out   dx, al; Low
	mov   al, ah
	out   dx, al; High
	pop   dx
	pop   ax
endm

I8253_Read macro port, countn
	push dx
	mov   dx, port + I8253_CONTROL
	xor   al, al
	out   dx, al
	mov   dx, port + countn
	in    al, dx; Low
	mov   ah, al
	in    al, dx; High
	pop   dx
endm
