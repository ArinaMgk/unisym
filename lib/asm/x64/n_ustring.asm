; ASCII [NASM & AASM] WinNT x64 RFC12
; x64-calling-convention arina-covenant
BITS 64
[SECTION .data]
	DB "UNISYM DOSCON.IO", 0, "[COPYRIGHT]Phina Ren", "[AUTHOR] Haruno Doshou"
[SECTION .code]
	GLOBAL ChrAddx64
	; EXTERN malloc
ChrAddx64:; E.g. `12` and `99` for `111`
	PUSH RBP
	MOV RBP, RSP
	SUB RSP, 0x08 * 10;{TEMP} 10 int64_t {inp a, inp b}
	;{this version}
	; - no sign digit
	; - the first parameter should be mutable
	; - each input string should terminate with double 0 for carry digit
	; - no intersection of inputs
	XOR RAX, RAX
	OR RCX, RCX
	JZ @CHRADD@ENDO
	OR RDX, RDX
	JZ @CHRADD@ENDO
	; make EDI->a(ones digit), ESI->b(ones digit)
	; then make sure length of a > length of b by swapping
	MOV RDI, RCX
	MOV RSI, RDX
	@CHRADD@LEN@A:
		CMP BYTE[RDI], 0
		JZ @CHRADD@LEN@A@END
		INC RDI
		JMP @CHRADD@LEN@A
	@CHRADD@LEN@A@END:
		DEC RDI
	@CHRADD@LEN@B:
		CMP BYTE[RSI], 0
		JZ @CHRADD@LEN@B@END
		INC RSI
		JMP @CHRADD@LEN@B
	@CHRADD@LEN@B@END:
		DEC RSI
	; exchange if (rdi - rcx) < (rsi - rdx) i.e. (rdi - rcx + rdx - rsi) < 0
	MOV RBX, RDI
	SUB RBX, RCX
	ADD RBX, RDX
	SUB RBX, RSI
	JNS @CHRADD@LEN@A@GT@B; a > b
		; exchange a and b
		XCHG RCX, RDX
		XCHG RDI, RSI
	@CHRADD@LEN@A@GT@B:
	MOV QWORD[RBP - 0x08 * (10 - 0)], RCX; a
	MOV QWORD[RBP - 0x08 * (10 - 1)], RDX; b
	MOV QWORD[RBP - 0x08 * (10 - 2)], RDI; endofa
	MOV RCX, RSI
	SUB RCX, RDX
	INC RCX; length of b
	JZ @CHRADD@ENDO; if (inp b)[0] is 0
	TEST RCX, RCX; Clear Carry Flag
	XOR RAX, RAX
	@CHRADD@STRADD@STAGE1:; cautious(carry)
		SHR RAX, 8
		MOV AL, [RSI]
		ADC AX, 0xC6; 0x100-0x40+6
		ADD [RDI], AL
		DEC RDI
		DEC RSI
		LOOP @CHRADD@STRADD@STAGE1
	PUSHF
	MOV RCX, QWORD[RBP - 0x08 * (10 - 2)]
	SUB RCX, QWORD[RBP - 0x08 * (10 - 0)]
	INC RCX
	POPF; Do not Clear Carry Flag
	@CHRADD@STRADD@CARRY:
		PUSHF
		CMP QWORD[RBP - 0x08 * (10 - 0)], RDI
		JBE @CHRADD@STRADD@CARRY@NEXT
			PUSH RDI
			MOV RDI, QWORD[RBP - 0x08 * (10 - 2)]
			CALL @CHRSHR
			POP RDI
		@CHRADD@STRADD@CARRY@NEXT:
		POPF
		ADC BYTE[RDI], 0
		MOV AL, 0x3A; CMP(dest,sors) Influence CF !!!
		SUB AL, [RDI]
		JA @CHRADD@STRADD@CARRY@NEXT@NEXT
		ADD BYTE[RDI], 0xF6; -10, also set CF
		@CHRADD@STRADD@CARRY@NEXT@NEXT:
		DEC RDI
		JC @CHRADD@STRADD@CARRY
	MOV RAX, QWORD[RBP - 0x08 * (10 - 0)]
	@CHRADD@ENDO:
		LEAVE; MOV RSP, RBP; POP RBP
		RET
@CHRSHR:; (volatile rightest RDI, times RCX)
	PUSH RAX
	PUSH RCX
	PUSHF
	@CHRSHR@LOOP:
		MOV AL, [RDI]
		MOV [RDI+1], AL
		DEC RDI
		LOOP @CHRSHR@LOOP
	MOV BYTE[RDI], 0x30
	POPF
	POP RCX
	POP RAX
	RET

ChrSubx64:
	; Use the greater one minus the less one, we can judge which is greater by comparing the pointer returned.
	;{TODO} ...
