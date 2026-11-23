; The branch target is intentionally outside the ARM32 branch range.

[ARCH arm32]
[SECTION .text]
B far_target

[SECTION .far vstart=0x02000008]
far_target:
NOP
