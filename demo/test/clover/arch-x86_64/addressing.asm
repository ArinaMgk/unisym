; Expected: exercise ModRM, SIB, displacement, extended registers, and RIP-relative addressing.

bits 64
default rel

section .text
	mov rax, [rbx]
	mov rcx, [rbp]
	mov rdx, [r12+r13*8+0x1234]
	mov [rsi+7], r8d
	lea r9, [rel target]
	ret

target:
	dq 0x8877665544332211

