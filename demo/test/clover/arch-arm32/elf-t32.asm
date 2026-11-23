; Verify T32 ELF branch relocations, mapping symbols, and function state.

[ARCH arm32]
[CORE cortex-m7]
[INST T32]
[BITS 32]
[SECTION .text]

GLOBAL entry:function
EXTERN external_target
entry:
B external_target
BNE external_target
BX LR
