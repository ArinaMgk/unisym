; Expected: emit initialized data and keep BSS reservation outside the flat payload.

bits 64

section .data
	db "AASM", 0
	dw 0x1234
	dd 0x89abcdef
	dq 0x0123456789abcdef
	times 4 db 0x5a
	align 16, db 0xcc

section .bss
	resb 3


