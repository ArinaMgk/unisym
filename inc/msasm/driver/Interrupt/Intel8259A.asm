; ASCII [MASM] TAB4 CRLF
; Attribute: MCU template
; LastCheck: RFX28
; AllAuthor: @dosconio
; ModuTitle: Intel Chip 8259A
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

I8259_Init macro port_imm:=<210h>
	cli
	mov al, 13h ; icw1
	mov dx, port_imm; 8259A
	out dx, al
	mov al, 8; icw2
	mov dx, port_imm + 1
	out dx, al
	mov al, 1; icw4
	out dx, al
endm

I8259_Regi macro int_rout, int_seg, intn:=<8>
	;{TODO} Magic Number: 8, Real-Mode
	cli
	push ax
	push dx
	push ds
	xor ax, ax
	mov ds, ax
	lea ax, int_rout
	mov ds:[4*intn], ax
	mov ax, int_seg
	mov ds:[4*intn+2], ax
	in  al, dx
	and al, 0feh
	out dx, al
	mov dx, 203h
	mov al, 80h
	out dx, al
	mov al, 1
	mov dx, 200h
	out dx, al
	pop ds
	pop dx
	pop ax
endm

;<CONTINUE> ?
; mov dx,210h
; mov al,20h
; out dx,al
