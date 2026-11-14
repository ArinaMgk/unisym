; ASCII AASM TAB4 LF
; ARM64 minimal integer, control-flow, and memory encoding coverage.

[ARCH arm64]
[BITS 64]

NOP
MOV X0, X1
MOV W2, 0x12345678
MOV X3, -1
ADD X4, X0, X1
ADD W5, W2, 16
SUB X6, X4, 4096
CMP X6, X4
CMP W5, 16
AND X7, X4, X6
ORR W8, W5, W2
EOR X9, X7, X6
STR X9, [SP + 16]
LDR X10, [SP + 16]
STRB W8, [SP + 1]
LDRB W11, [SP + 1]
LDRSB X12, [SP + 1]
MOV X13, BYTE [SP + 1]
MOV X14, SIGNED BYTE [SP + 1]
MOV BYT8 [SP + 24], X10
B.EQ equal
MOV W15, 0
equal:
BL call_target
JMP done
call_target:
RET
done:
CALL X10
BR X9
BLR X8
RET LR
