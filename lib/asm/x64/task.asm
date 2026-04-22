[BITS 64]

GLOBAL SwitchTaskContext; (NormalTaskContext* nex, NormalTaskContext* crt)
SwitchTaskContext:
	MOV [RSI + 0x00], RAX
	MOV [RSI + 0x08], RCX
	MOV [RSI + 0x10], RDX
	MOV [RSI + 0x18], RBX
	LEA RAX, [RSP + 8]; skip RETADDR
	MOV [RSI + 0x20], RAX  ; RSP
	MOV [RSI + 0x28], RBP
	MOV [RSI + 0x30], RSI
	MOV [RSI + 0x38], RDI
	MOV [RSI + 0x40], R8
	MOV [RSI + 0x48], R9
	MOV [RSI + 0x50], R10
	MOV [RSI + 0x58], R11
	MOV [RSI + 0x60], R12
	MOV [RSI + 0x68], R13
	MOV [RSI + 0x70], R14
	MOV [RSI + 0x78], R15
	;
	MOV RAX, [RSP]; RETADDR
	MOV [RSI + 0x80], RAX  ; RIP
	PUSHFQ
	POP QWORD [RSI + 0x88] ; RFLAGS
	MOV RAX, CR3
	MOV [RSI + 0x90], RAX  ; CR3
	;
    MOV AX, CS
    MOV [RSI + 0xA0], AX
    MOV AX, DS
    MOV [RSI + 0xA2], AX
    MOV AX, ES
    MOV [RSI + 0xA4], AX
    MOV AX, SS
    MOV [RSI + 0xA6], AX
	MOV AX, FS
	MOV [RSI + 0xA8], AX
	MOV AX, GS
	MOV [RSI + 0xAA], AX
	FXSAVE [RSI + 0xB0]
	;
	MOV QWORD [RSI + 0x2C8], 0 ; clear crt->just_schedule
	;
	; IRET Stack Frame
	; CPU0
	MOV RSP, 0xFFFF_FFFFFFFFFFF8
	XOR RAX, RAX
	MOV AX, [RDI + 0xA6]
	PUSH RAX; SS
    PUSH QWORD [RDI + 0x20] ; RSP
    PUSH QWORD [RDI + 0x88] ; RFLAGS
	MOV AX, [RDI + 0xA0]
    PUSH RAX ; CS
    PUSH QWORD [RDI + 0x80] ; RIP
	;
	FXRSTOR [RDI + 0xB0]
	; LONG64 take DEFG zero OK
	MOV AX, 0
	MOV DS, AX
	MOV ES, AX
	MOV FS, AX
	MOV GS, AX
	;
	MOV RAX, [RDI + 0x00]
	MOV RCX, [RDI + 0x08]
	MOV RDX, [RDI + 0x10]
	MOV RBX, [RDI + 0x18]
	MOV RBP, [RDI + 0x28]
	MOV RSI, [RDI + 0x30]
	MOV R8,  [RDI + 0x40]
	MOV R9,  [RDI + 0x48]
	MOV R10, [RDI + 0x50]
	MOV R11, [RDI + 0x58]
	MOV R12, [RDI + 0x60]
	MOV R13, [RDI + 0x68]
	MOV R14, [RDI + 0x70]
	MOV R15, [RDI + 0x78]
	PUSH RAX
	MOV RAX, [RDI + 0x90]; CR3
	MOV RDI, [RDI + 0x38]
	
	PUSH RCX
	MOV RCX, CR3
	CMP RCX, RAX
	POP RCX
	JE .skip_cr3
	MOV CR3, RAX
.skip_cr3:
	
	POP RAX; Common Kernel Stack Mapping
	O64 IRET



