; Verify a call fixup to a label in another section.

[ARCH arm32]
[SECTION .text]
CALL target

[SECTION .other]
target:
RET
