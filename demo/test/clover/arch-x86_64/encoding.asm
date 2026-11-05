; Expected: encode representative register operations and fixed BYTn operand widths.

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
	mov BYTE [rax], bl
	mov BYT2 [rax], bx
	mov BYT4 [rax], ebx
	mov BYT8 [rax], rbx
	fld BYT10 [rax]
	movdqu xmm0, BYT16 [rax]
	vmovdqu ymm0, BYT32 [rax]
	ret

