; ASCII Arina 
; Ultimate version at 2022 June. Opensource since 2023 Aug28
;	Copyright 2023 ArinaMgk
;	
;	Licensed under the Apache License, Version 2.0 (the "License");
;	you may not use this file except in compliance with the License.
;	You may obtain a copy of the License at
;	
;	http://www.apache.org/licenses/LICENSE-2.0
;	
;	Unless required by applicable law or agreed to in writing, software
;	distributed under the License is distributed on an "AS IS" BASIS,
;	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;	See the License for the specific language governing permissions and
;	limitations under the License.
;
%include "timer.a"
%include "pseudo.a"

File
JMP	PROG
	VAR:	DW	0x0000
	thism   db	0x00
	VARMON	DB	0x00
	VARWIK  DB	0X0000
	STR1	DB	'Month : ** Year : ****'
	STR2	DB	'PastDays :'
	STR3	DB	'WeekCode :'
	STR4	DB	'Sun.Mon.Tue.Wed.Thr.Fri.Sat.'
SYM:
	BARCOL	DB	0X70
	NRMCOL	DB	0X07
	LINCOL	DB	0X0D
	YEWCOL	DB	0X06
	TODAY:  DB 0X01
PROG:

MOV AX,CS
MOV DS,AX
MOV AX,0XB800
MOV ES,AX
MOV AX,2023
MOV CX,4
;herday - 201401 : 4 whole days
GetMoexDayIdentity AX,CX
MOV WORD[VAR],BX
MOV BYTE[thism],DL

XOR	SI,SI
MOV	CH,10
MOV	DI,AX
@LUP1:
DIV	CH
NEG	SI
ADD	AH,0X30
MOV	[SI+STR1+21],AH
XOR	AH,AH
NEG	SI
INC SI
CMP	SI,4
JB	@LUP1
MOV	BYTE[VARMON],CL
XOR	CH,CH
MOV	AX,CX
MOV	CH,10
DIV	CH
ADD	AH,0X30
ADD	AL,0X30
MOV	[STR1+9],AH
MOV	[STR1+8],AL
@PRT:
MOV AL,BYTE[VARMON]
XOR AH,AH
ADD BX,AX
MOV AL,BYTE[BX]
MOV BYTE[VARMON],AL
MOV	SI,STR1
MOV	DI,STR2
XOR	BP,BP
LUP2:
MOV	AL,byte[SI]
INC	SI
CMP	SI,DI
JA	END_LUP2
MOV	byte[ES:BP],AL
INC	BP
MOV	byte[ES:BP],0x07
INC	BP
JMP	LUP2
END_LUP2:
MOV	AX,ES
ADD	AX,10
MOV	ES,AX
MOV	SI,STR2
MOV	DI,STR3
XOR	BP,BP
LUP3:
MOV	AL,byte[SI]
INC	SI
CMP	SI,DI
JA	END_LUP3
MOV	byte[ES:BP],AL
INC	BP
MOV	byte[ES:BP],0x07
INC	BP
JMP	LUP3
END_LUP3:
MOV	DX,0XF000
MOV	CL,4+4+4
COUNTYEARDAY:
MOV	AX,WORD[VAR]
AND	AX,DX
SHR	AX,CL
CMP	AL,10
JB	LABEL1
ADD	AL,'A'-10
JMP	LABEL2	
LABEL1:
ADD	AL,0X30
LABEL2:
MOV	byte[ES:BP],AL
ADD	BP,2
SHR	DX,4
CMP	CL,0
JE 	@STR3_
SUB	CL,4
JMP	COUNTYEARDAY
@STR3_ :
MOV	AX,ES          
ADD	AX,10
MOV	ES,AX
MOV	SI,STR3
MOV	DI,STR4
XOR	BP,BP
LUP4:
MOV	AL,byte[SI]
INC	SI
CMP	SI,DI
JA	END_LUP4
MOV	byte[ES:BP],AL
INC	BP
MOV	byte[ES:BP],0x07
INC	BP
JMP	LUP4
END_LUP4:
XOR	DX,DX
MOV	AX,word[VAR]
MOV	BX,7
DIV	BX
MOV     WORD[VARWIK],AX
MOV	DX,0XF000
MOV	CL,4+4+4
COUNTYEARDAY2:
MOV     AX,WORD[VARWIK]
AND	AX,DX
SHR	AX,CL		
CMP	AL,10
JB	LABEL3
ADD	AL,'A'-10
JMP	LABEL4	
LABEL3:
ADD	AL,0X30
LABEL4:
MOV	byte[ES:BP],AL
ADD	BP,2
SHR	DX,4
CMP	CL,0
JE 	@STR4_
SUB	CL,4
JMP	COUNTYEARDAY2
@STR4_:
MOV	AX,ES
ADD	AX,10
MOV	ES,AX
MOV	SI,STR4
MOV	DI,SYM
XOR	BP,BP
LUP5:
MOV	AL,byte[SI]
INC	SI
CMP	SI,DI
JA	END_LUP5
MOV	byte[ES:BP],AL
INC	BP
MOV	byte[ES:BP],0x07
INC	BP
JMP	LUP5
END_LUP5:
@PRT_FIRST_LINE:
MOV	AX,ES
ADD	AX,10
MOV	ES,AX
MOV     AX,WORD[VAR]

PUSHA
MOV CX,4*6
XOR BP,BP
CLS_:
MOV	byte[ES:BP],' '
INC	BP
MOV	byte[ES:BP],0x07
INC	BP
LOOP CLS_
POPA

DEC     AX
MOV     CX,7
DIV     CX
MOV     AX,DX
MOV     SI,AX
MOV     CL,8
MUL     CL
MOV     BP,AX
MOV     DI,0
XOR     CX,CX
MOV     CL,BYTE[thism]
JMP     @PRT_DATE_LUP
@PRT_DATE_LUP:
MOV     AX,WORD[VAR]
XOR     DX,DX
MOV     DL,BYTE[DS:TODAY]
CMP     DL,CL
JA      @END
ADD     AX,DX
XOR     DX,DX
MOV     CH,0X70
MOV     BX,30
DIV     BX
CMP     DX,5
JAE     @PRT_DATE_LUP_NORMALDAY
MOV     CH,0X78
@PRT_DATE_LUP_NORMALDAY:
MOV     BYTE[ES:BP],'['
INC     BP
MOV     BYTE[ES:BP],CH
INC     BP
MOV     AL,BYTE[TODAY]
XOR     AH,AH
MOV     BL,10
DIV     BL
ADD     AH,0X30
ADD     AL,0X30
MOV     BYTE[ES:BP],AL
INC     BP
MOV     BYTE[ES:BP],CH
INC     BP
MOV     BYTE[ES:BP],AH
INC     BP
MOV     BYTE[ES:BP],CH
INC     BP
MOV     BYTE[ES:BP],']'
INC     BP
MOV     BYTE[ES:BP],CH
INC     BP
ADD     BYTE[TODAY],1
CMP     SI,6
JE      NEW_COL
INC     SI
JMP     @PRT_DATE_LUP
NEW_COL:
INC     DI
MOV	AX,ES
ADD	AX,10
MOV	ES,AX
XOR SI,SI
XOR BP,BP
JMP @PRT_DATE_LUP
@END:
STI
FIN	HLT
JMP	FIN
Endf