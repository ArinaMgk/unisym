; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86) Format(ELF)
; LastCheck: 2024Feb18
; AllAuthor: @dosconio
; ModuTitle: 
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

GLOBAL _HALT, HALT
GLOBAL _InterruptDisable, InterruptDTabLoad
GLOBAL  InterruptEnable,  InterruptDisable
GLOBAL getCR3, getEflags
GLOBAL jmpFar, jmpTask, CallFar
GLOBAL _returnfar
GLOBAL returnfar
GLOBAL TaskReturn

[CPU 386]

[BITS 32]


section .text

_HALT:
HALT:
	HLT
RET

InterruptEnable:
	STI
RET

_InterruptDisable:
InterruptDisable:
	CLI
RET

InterruptDTabLoad:
	; - [EBP+4*0]=EBX
	; - [EBP+4*1]=Return Address
	; - [EBP+4*2]=Address of IDT
	CLI
	PUSH EBX
	XCHG ESP, EBP
	MOV  EBX, [EBP+4*2]
	LIDT [EBX]
	XCHG ESP, EBP
	POP  EBX
RET

getCR3:
	MOV EAX, CR3
RET

getEflags:
	PUSHFD
	POP EAX
RET

; ---- ---- JMPs ---- ---- ;

;struct __attribute__((packed)) tmp48be_t { uint32 u_32fore; uint16 u_16back; };
jmpFar:; + TSS_ID_inGDT
	; - [EBP+4*0]=BP
	; - [EBP+4*1]=Return Address
	; - [EBP+4*2]=Address
	; - [EBP+4*3]=Segment(word-eff, dword-len)
	PUSH EBP
	MOV EBP, ESP
	JMP FAR [EBP+4*2]
	MOV ESP, EBP
	POP EBP
RET

__GET_DESC_BASE:; EAX(&DESC)->EBX(BASE). USE CB
	MOV  EBX, [EAX+4]; B[Base8][*8][*8][bAse8]
	MOV  ECX, EBX    ; C[Base8][*8][*8][bAse8]
	AND  BX, 0x00FF  ; B[Base8][*8]    [bAse8]
	SHR  ECX, 16     ; C           [Base8][*8]
	AND  CX, 0xFF00  ; C           [Base8]
	ADD  BX, CX      ; B           [BAse16   ]
	SHL  EBX, 16     ; B[BAse16   ]
	MOV  ECX, [EAX+0]; C[baSE16   ][Limi16   ]
	SHR  ECX, 16     ; C           [baSE16   ]
	OR   EBX, ECX
RET
jmpTask:; (TSS_ID(times of 8) )
	; - [EBP+4*0]=TSS.EBP
	; - [EBP+4*1]=TSS.EIP
	; - [EBP+4*2]=PARA.TSS_ID
	; x [EBP+4*3]=PARA.OLD_ID
	; Caution: Stack Balance
		CLI; reset IF again
		PUSH EBP
		MOV  EBP, ESP; EBP+4*2 = TSS.ESP
		;
		MOV  [jmpTask_var+4*0], EAX; jmpTask_var[ 0] = TSS.EAX
		MOV  [jmpTask_var+4*1], ECX; jmpTask_var[ 4] = TSS.ECX
		MOV  [jmpTask_var+4*2], EDX; jmpTask_var[ 8] = TSS.EDX
		MOV  [jmpTask_var+4*3], EBX; jmpTask_var[12] = TSS.EBX
		PUSHFD; [1]+
	; [save olds] make EBX -> Old TSS
		STR  CX
		; MOV  ECX, [EBP+4*3]
		AND  ECX, 0x0000FFF8
		SGDT [jmpTask_var+4*4]
		MOV  EAX, [jmpTask_var+4*4+2]; skip u16 limit for addr32/64
		ADD  EAX, ECX
		MOV  EDX, EAX
		CALL __GET_DESC_BASE
	; [save olds] keep CDB
		MOV EAX, CR3
		MOV [EBX+28], EAX; PDBR
		MOV EAX, [EBP+4*1]
		MOV [EBX+32], EAX; EIP
		POP EAX; [1]-
		MOV [EBX+36], EAX; EFLAGS
		MOV EAX, [jmpTask_var+4*0]
		MOV [EBX+40], EAX; EAX
		MOV EAX, [jmpTask_var+4*1]
		MOV [EBX+44], EAX; ECX
		MOV EAX, [jmpTask_var+4*2]
		MOV [EBX+48], EAX; EDX
		MOV EAX, [jmpTask_var+4*3]
		MOV [EBX+52], EAX; EBX
		MOV EAX, EBP
		ADD EAX, 4*2
		MOV [EBX+56], EAX; ESP
		MOV EAX, [EBP+4*0]
		MOV [EBX+60], EAX; EBP
		MOV [EBX+64], ESI; ESI
		MOV [EBX+68], EDI; EDI
		MOV EAX, ES
		MOV [EBX+72], EAX; ES
		MOV EAX, CS
		MOV [EBX+76], EAX; CS
		MOV EAX, SS
		MOV [EBX+80], EAX; SS
		MOV EAX, DS
		MOV [EBX+84], EAX; DS
		MOV EAX, FS
		MOV [EBX+88], EAX; FS
		MOV EAX, GS
		MOV [EBX+92], EAX; GS
		; skip LDT
	; [load news] apply TSS
		MOV  EAX, [jmpTask_var+4*4+2]; skip u16 limit for addr32/64
		MOV  ECX, [EBP+4*2]
		ADD  EAX, ECX
		LTR  CX  
		AND  BYTE[EDX+5], 0xFD; ! BUSY-BIT can only be reset by hardware normally
		LGDT [jmpTask_var+4*4]
		CALL __GET_DESC_BASE
			OR  EBX, 0x80000000; MCCA-USED
		MOV  AX, [EBX+96]; LDT
		LLDT AX
			OR  ESP, 0x80000000; MCCA-USED
		MOV EAX, [EBX+92]; GS
		MOV GS, EAX
		MOV EAX, [EBX+88]; FS
		MOV FS, EAX
		MOV EAX, [EBX+84]; DS
		MOV DS, EAX
		MOV EAX, [EBX+72]; ES
		MOV ES, EAX
		MOV EAX, [EBX+68]; EDI
		MOV EDI, EAX
		MOV EAX, [EBX+64]; ESI
		MOV ESI, EAX
		MOV EAX, [EBX+60]; EBP
		MOV EBP, EAX
		; SS ESP CS EIP FLAG ACBD
		MOV EAX, [EBX+80];
		AND EAX, 0x3
		MOV ECX, SS
		AND ECX, 0x3
		CMP EAX, ECX
		JZ paral_jump
		PUSH DWORD [EBX+80]; SS
		PUSH DWORD [EBX+56]; ESP
		JMP jtask_end
		paral_jump:
		MOV ECX, [EBX+80]
		MOV SS, ECX
		MOV ESP, [EBX+56]
		jtask_end:
		PUSH DWORD [EBX+36]; EFLAGS
		PUSH DWORD [EBX+76]; CS
		PUSH DWORD [EBX+32]; EIP
		MOV EAX, [EBX+28]
		MOV CR3, EAX; PDBR
		MOV EAX, [EBX+40]; EAX
		MOV ECX, [EBX+44]; ECX
		MOV EDX, [EBX+48]; EDX
		MOV EBX, [EBX+52]; EBX
		IRET; POP EIP CS EFLAGS ESP SS
	; unreachable
	MOV  ESP, EBP
	POP  EBP
RET


CallFar:
	PUSH EBP
	MOV EBP, ESP
	CALL FAR [EBP+4*2]
	MOV ESP, EBP
	POP EBP
RET

_returnfar:
returnfar:
	RETF
RET
ALIGN 16
TaskReturn:
	PUSHFD
	POP EDX
	TEST DX, 0100_0000_0000_0000B
	JNZ NESTED_TASK
	JMP 8*5:0; 8*5 TEMP
RET
	NESTED_TASK: IRETD
RET; for next calling the subapp
ALIGN 16


section .data

jmpTask_var: DD 0,0,0,0, 0,0,0,0; do not access after change page

