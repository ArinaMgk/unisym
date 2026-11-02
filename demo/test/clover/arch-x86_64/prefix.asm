; Expected: encode lock, repeat, segment, operand-size, and address-size prefixes.

bits 64

section .text
	lock inc dword [rax]
	rep movsb
	mov rax, [fs:0x20]
	mov ax, bx
	a32 mov eax, [eax+4]
	ret

