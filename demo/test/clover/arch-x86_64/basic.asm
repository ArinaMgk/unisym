; Expected: produce one global text symbol, one local data symbol, and stable flat bytes.

bits 64

global aasm_arch_x86_64_basic

section .text
aasm_arch_x86_64_basic:
	mov rax, 1
	add rax, 2
	cmp rax, 3
	jne .failed
	xor eax, eax
	ret

.failed:
	mov eax, 1
	ret

section .rodata
aasm_arch_x86_64_value:
	dq 0x0123456789abcdef
