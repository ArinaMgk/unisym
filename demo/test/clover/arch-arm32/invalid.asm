; Inputs expected to produce ARM32 operand diagnostics.

[ARCH arm32]
[BITS 32]

MOV.S R0, 1
ADD R15, R0, R1
LDR R0, [PC]
LDR R0, [R1 + 4096]
LDRH R0, [R1 + 256]
ADD.S.EQ R0, R1, R2
B [target]
target:
NOP
