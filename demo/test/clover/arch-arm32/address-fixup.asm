; Verify split ARM32 label-address fixups across distant sections.

[ARCH arm32]
[SECTION .text]
MOV R2, far_data + 0x678

[SECTION .data vstart=0x12345000]
far_data:
DQ 0
