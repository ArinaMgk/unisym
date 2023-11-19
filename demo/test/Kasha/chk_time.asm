; Contributed by @dosconio RFQ22:10:43~RFE29
; Follow ArinaMgk's style
; Do well on VMBox and VMware but Bochs
%include "video.a"
%include "pseudo.a"
%include "timer.a"
%include "arith.a"

File
	DefineStack16
	MOV AX,CS
	MOV DS,AX
	MOV AX,0xB800
	ADD AX,80*2/16; NEW LINE
	MOV ES,AX

	;[H][H][:][M][M][:][S][S][!]
	;0  1  2  3  4  5  6  7  8
	TimerInit16 CS,INT70H
	MOV BYTE[ES:2*2],':'
	OR BYTE[ES:2*2+1],0x80; KIRA
	MOV BYTE[ES:5*2],':'
	OR BYTE[ES:5*2+1],0x80; KIRA
	MOV BYTE[ES:8*2],'!'

	LUP:
	HLT
	; NOT BYTE[ES:8*2]
	JMP LUP
F_BCD2A:
	BCD2ASCII
	RET
INT70H:
	PUSHAD
	TimerReadTime; AL:DL:DH

	CALL F_BCD2A
	MOV BYTE[ES:0*2],AH
	MOV BYTE[ES:1*2],AL
	;NOT BYTE[ES:2*2+1]

	MOV AL,DL
	CALL F_BCD2A
	MOV BYTE[ES:3*2],AH
	MOV BYTE[ES:4*2],AL
	;NOT BYTE[ES:5*2+1]

	MOV AL,DH
	CALL F_BCD2A
	MOV BYTE[ES:6*2],AH
	MOV BYTE[ES:7*2],AL
	
	MOV AL,0X20; SEND EOI
	OUT 0XA0,AL; SENDTO SLAVE
	OUT 0X20,AL; SENDTO MASTER
	POPAD
	IRET
Endf

