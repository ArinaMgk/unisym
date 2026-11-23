; Verify ARM32 local and cross-section branch/address fixups.

[ARCH arm32]
[BITS 32]
[SECTION .text]

start:
B local
MOV R0, data_label
local:
BL worker
JMP data_code
worker:
MOV R1, data_label + 4
RET

[SECTION .data]
data_label:
DD 0
data_code:
NOP
