; Contributed by @dosconio for n_video.a RFE22:15:00:00
; Follow ArinaMgk's style

	[CPU 8086]
	[BITS 16]

%include "pseudo.a"
%include "video.a"

File
	MOV AX,0XB800
	MOV ES,AX
	MOV AX,0x0100
	MOV DS,AX

	MOV AL,'A'
	MOV AH,0X70
	XOR BX,BX
	CALL F_PRINTCHAR

	MOV BX,80; COL0 ROW1
	CALL F_SETCUR
	MOV SI,HELLO
	MOV AH,0x70
	CALL F_PRINT

	CALL F_GETCUR
	ADD BX,2
	CALL F_SETCUR
	MOV AL,'B'
	MOV AH,0X70
	CALL F_PRINTCHAR

	CALL F_ROLL

	CLI
	HLT
	F_GETCUR:
		ConCursor
		MOV BX,AX
		RET
	F_SETCUR:
		ConCursor BX
		RET
	F_PRINTCHAR:
		ConPrintChar AL,AH,BX
		RET
	F_PRINT:
		ConPrint SI,AH
		RET
	F_ROLL:
		ConRoll; 1
		RET
	HELLO: DB "Hello,world!",0x0A,0x0D,0
Endf
