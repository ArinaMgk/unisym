; ASCII [MASM] TAB4 CRLF
; Attribute: MCU template
; LastCheck: RFX28
; AllAuthor: @dosconio
; ModuTitle: Delay
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

DelayLoop macro loop_times:<01f00h>
	push cx
	mov cx, loop_times
	loop $
	pop cx
endm
