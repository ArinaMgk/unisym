; ASCII AASM TAB4 LF
; Cortex-M7 T32 16-bit encoding regression sample.

[ARCH arm32]
[CORE cortex-m7]
[INST t32]
[INST T32]
[BITS 32]

MOVS R0, 1
ADDS R0, R0, 1
SUBS R1, R1, 2
CMP R0, 1
ORRS R0, R0, R1
STR R0, [R1 + 4]
LDR R2, [R1 + 4]
BHS finished
MOVS R3, 5
finished:
NOP
BX LR
