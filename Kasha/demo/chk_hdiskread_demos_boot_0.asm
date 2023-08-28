; Contributed by @dosconio
; Follow ArinaMgk's style
	[CPU 386]

%include "hdisk.a"
%include "video.a"

CLI
MOV AX, 0
MOV SS, AX
MOV SP, 0X7C00
STI

MOV AX, 0X07C0; CS==0 USUALLY
MOV DS, AX
MOV AX, 0XB800
MOV ES, AX
MOV SI, str

PrintStr:
MOV AL, [SI]
INC SI
OR AL, AL
JZ ReadKey
ConPrintChar AL, 0x70, ~; [Black on White]
JMP PrintStr

ReadKey:
; KeyRead
	XOR AH, AH
	INT 0x16
MOV AX, 0X0100
MOV ES, AX
MOV DS, AX

XOR EBX, EBX
MOV EAX, 1
MOV CL, 1
CALL F_ReadDisk
MOV CL, [0]
DEC CL
JZ fin
MOV EAX, 2
CALL F_ReadDisk

fin:
jmp 0x0100: 0x1
str: DB 'Coge Vessel (x86) ',__DATE__," : Press any key to continue...",0
F_ReadDisk:
	HdiskLoadLBA28 EAX,CL
	RET
times 510-$+$$ db 0
arina