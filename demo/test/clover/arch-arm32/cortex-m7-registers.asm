; Verify Cortex-M7 T32 high-register transfers and memory operations.

[ARCH arm32]
[CORE cortex-m7]
[INST T32]
[BITS 32]

MOV R2, SP
MOV R3, LR
STR R4, [R0 + 8]
STR R8, [R0 + 24]
STR R11, [R0 + 36]
LDR R4, [R2 + 8]
LDR R8, [R2 + 24]
LDR R11, [R2 + 36]
MOV SP, R1
BX R3
