; ASCII NASM TAB4 CRLF
; Attribute: CPU(x86)
; LastCheck: 2024Feb15
; AllAuthor: @dosconio
; ModuTitle: Interruptor Chip - i8259A
; Copyright: ArinaMgk UniSym, Apache License Version 2.0

GLOBAL i8259A_Init32

[CPU 386]

[BITS 16]
	;{}...
	RET

[BITS 32]

;{TEMP} only for a timer
;      - {WAIT} abstracted from Mecocoa

i8259A_Init32:
	; Master PIC
	MOV AL, 10001B; ICW1 {ICW1EN, EdgeTrigger, 8b-ADI, !Single, ICW4EN}
	OUT 0X20, AL
	MOV AL, 0X20; ICW2: Relative Vector Address
	OUT 0X21, AL
	MOV AL, 00000100B; ICW3: P-2 Connected to Slave
	OUT 0X21, AL
	MOV AL, 00001B; ICW4 {!SFNM, !BUF, Slave(Unused?), ManualEOI, Not8B}
	OUT 0X21, AL
	; Slave PIC
	MOV AL, 10001B; ICW1 {ICW1EN, EdgeTrigger, 8b-ADI, !Single, ICW4EN}
	OUT 0XA0, AL
	MOV AL, 0X70; ICW2: Relative Vector Address
	OUT 0XA1, AL
	MOV AL, 2; ICW3: P-2 Connected to Slave
	OUT 0XA1, AL
	MOV AL, 00001B; ICW4 {!SFNM, !BUF, Slave(Unused?), ManualEOI, Not8B}
	OUT 0XA1, AL
	;{TODO move to Timer} Enable Timer
	MOV AL, 0X0B; RTC Register B
	OR AL, 0X80; Block NMI
	OUT 0X70, AL
	MOV AL, 0X12;Set Reg-B {Ban Periodic, Open Update-Int, BCD, 24h}
	OUT 0X71, AL;
	IN AL, 0XA1; Read IMR of Slave PIC
	AND AL, 0XFE; Clear BIT 0, which is linked with RTC
	OUT 0XA1, AL; Write back to IMR
	MOV AL, 0X0C
	OUT 0X70, AL
	IN AL, 0X71; Read Reg-C, reset pending interrupt
	;{TODO} Check PIC Device?
	PIC_CHECK:
	RET
