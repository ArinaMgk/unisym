; Expected: encode representative 8-, 32-, and 64-bit register operations.

bits 64

section .text
	mov rax, 0x1122334455667788
	mov r8, rax
	add r9, 0x7f
	sub r10, -1
	xor eax, eax
	shl r11, 1
	push r12
	pop r13
	ret

