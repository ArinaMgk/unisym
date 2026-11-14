; ASCII AASM TAB4 LF
; ARM32 A32 minimal integer, control-flow, and memory encoding coverage.

[ARCH arm32]
[BITS 32]

NOP
MOV R0, R1
MOV R2, 0x12345678
MOV R3, -1
ADD R4, R0, R1
ADD.S R5, R4, 16
SUB.NE R6, R5, R1, LSL 2
CMP R6, 0x80000000
AND R7, R4, R6
ORR R8, R5, R2, ROR 8
EOR R9, R7, R6
STR R9, [SP + 4]
LDR R10, [SP + 4]
STRB R8, [SP + 1]
LDRB R11, [SP + 1]
STRH R8, [SP - 2]
LDRH R12, [SP - 2]
LDRSB R0, [SP + 1]
LDRSH R1, [SP + 2]
MOV R12, BYTE [SP + 1]
MOV R0, SIGNED BYTE [SP + 1]
MOV BYT2 [SP - 2], R8
B.EQ equal
MOV R0, 0
equal:
BL call_target
JMP done
call_target:
RET
done:
CALL R10
JMP R9
BLX R8
RET
