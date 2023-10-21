; ASCII [MASM] for MSDOS/WIN16 RFC21
; rename this for less filename to adapt MASM
include INT21.ASM; INC File
DATA SEGMENT
	DW ?
DATA ENDS

STACK SEGMENT
	STAK DW 32 DUP(?)
	TOP DW ?
STACK ENDS

CODE SEGMENT
	ASSUME CS:CODE, DS:DATA, SS:STACK
	mov ax, DATA
	mov ds, ax
	mov ax, STACK
	mov ss, ax
	mov sp, TOP
	lup:
		ConGetc
		cmp al, 20h
		jz lup
		cmp al, 'q'
		jz endo
		ConPrintChar AL
		jmp lup
	endo:
		ConTerm
CODE ENDS

END
