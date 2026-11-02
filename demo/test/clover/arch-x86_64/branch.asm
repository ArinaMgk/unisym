; Expected: -O1 keeps conservative branches while -Ox selects shorter encodings.

bits 64

section .text
branch_start:
	jmp branch_forward
	times 120 nop

branch_forward:
	jne branch_far
	times 128 nop

branch_far:
	call branch_target

branch_target:
	ret

