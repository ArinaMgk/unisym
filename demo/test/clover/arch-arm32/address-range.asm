; The label address is intentionally outside the ARM32 address range.

[ARCH arm32]
[SECTION .text]
MOV R0, far_data

[SECTION .far vstart=0x100000000]
far_data:
DQ 0
