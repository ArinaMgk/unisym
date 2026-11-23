; Verify ARM32 A32 ELF symbols, relocations, and section output.

[ARCH arm32]
[BITS 32]
[SECTION .text]

GLOBAL entry
EXTERN external_call
entry:
CALL external_call
BL.NE external_call
B other
MOV R0, data_item + 4
DD data_item
NOP

[SECTION .text.other exec]
other:
RET

[SECTION .data]
data_item:
DD other
