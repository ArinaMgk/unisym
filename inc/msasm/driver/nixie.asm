; ASCII [MASM] TAB4 CRLF
; Attribute: MCU template
; LastCheck: RFX28
; AllAuthor: @dosconio
; ModuTitle: Nixie Tube
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

NixieArray db 3fh,06h,5bh,4fh, 66h,6dh,7dh,07h, 7fh,6fh,77h,7ch, 39h,5eh,79h,71h

z8279    equ       212h
d8279    equ       210h
ledmod   equ       00  ; 左边输入,八位显示外部译码八位显示
ledfeq   equ       38h ; 扫描频率

NixieInit macro
	mov    dx,z8279
	mov    al,ledmod
	out    dx,al
	mov    al,ledfeq
	out    dx,al
	mov    cx,08h; If you set 06h, the next 2 send `out d8279, ??h` will set the digit-6 and digit-7
	mov    dx,d8279
	mov    al,00h; all off
	out    dx,al
	loop   xz
endm

NixieSet macro
	mov    dx,z8279
	mov    al,81h
	out    dx,al
	pop    ax
	lea    bx,led
	xlat
	mov    dx,d8279
	out    dx,al
endm
