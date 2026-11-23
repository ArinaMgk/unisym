; Verify case-insensitive ARM32 instruction parsing.

[ARCH arm32]
[BITS 32]

mov r0, r1
add.eq.s r2, r2, r3
mov.eq r4, 0x12345678
ret
