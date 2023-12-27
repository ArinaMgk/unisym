; ASCII TAB4 CRLF
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

;TEST PROGRAM
;E:\CNRV\vdkcpy.exe "E:\PROJ\KASHA\KASHADOS[RE]\boot.o" "E:\PROJ\KASHA\KASHADOS[RE]\boot.floppy.img" 0
;E:\CNRV\vdkcpy.exe "E:\PROJ\KASHA\KASHADOS[RE]\test.o" "E:\PROJ\KASHA\KASHADOS[RE]\boot.floppy.img" 1
DB 0x01

JMP ENTRY
STR_1 DB 'KASHA DOS - TEST : HELLO WORLD! - ARINA RET29 ~',0x0D,0x0A,'Here is Floppy~',0
ENTRY:
	MOV SI,STR_1
	MOV DI,ENTRY
	MOV AX,0X0800
	MOV DS,AX
	XOR BX,BX
	MOV AH,0X0E
	@LUP:
	MOV AL,BYTE[SI]
	INC SI
	CMP SI,DI
	INT 0x10
	JAE AllEnd
	JMP @LUP
	AllEnd:HLT
	JMP AllEnd
TIMES 512-$+$$ DB 0