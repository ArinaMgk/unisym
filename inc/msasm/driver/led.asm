; ASCII [MASM] TAB4 CRLF
; Attribute: MCU template
; LastCheck: RFX28
; AllAuthor: @dosconio
; ModuTitle: LED (temp for 4 2-dim LEDS)
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

; for nega(G)-nega(R)-logi into al-posilogi. E.g. `11111110` is let red LED_0 light and others off.
; DH means Hong and DL means Lv in Pinyin.

; AL <<< LEDStruct <<< Reds(DH), Greens(DL)
LED_Struct macro Red:<DH>, Green:<DL>
	push dx
	push cx
	mov dh, Red
	mov dl, Green
	mov al, dh
	and al, 00001111B
	mov cl, 4
	shl dl, cl; Green zo yo high part
	or al, dl
	not al
	pop cx
	pop dx
endm

; [Without Check]
; Reds(DH), Greens(DL) <<< LEDGet <<< AL
LEDGet macro; {AL}
	push cx
	push ax
	not al
	mov cl, 4
	mov dh, al
	and dh, 00001111B
	mov dl, al
	shr dl, cl
	pop ax
	pop cx
endm

; void <<< LEDSet <<< void
LEDSet macro
	push dx
	mov dx, 00200H; Here set your LED port address
	out dx, al
	pop dx
endm

