; Provide the external T32 branch target for the ELF link test.

[ARCH arm32]
[CORE cortex-m7]
[INST T32]
[BITS 32]
[SECTION .text]

GLOBAL external_target:function
external_target:
BX LR
