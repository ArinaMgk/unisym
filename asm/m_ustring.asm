; ASCII [MASM] for MSDOS/WIN16 (C)(ARINAMGK)UNISYM&DOSCONIO

; COMPARE TWO STRINGS TERMINATED WITH 0 AND RETURN THE DIFFERENCE IN AL
; AX[AL] <<< StrCompare <<< STR1=DI, STR2=SI
StrCompare MACRO STR1:=<di>, STR2:=<si>
	push si
	push di
	mov di, STR1
	mov si, STR2
	.lup:
		mov al, [di]
		mov ah, [si]
		or al, al
		jz .end
		or ah, ah
		jz .end
		cmp al, ah
		jne .end
		inc di
		inc si
		jmp .lup
	.end:
	sub al, ah
	pop di
	pop si
ENDM


