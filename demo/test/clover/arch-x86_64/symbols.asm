; Expected: resolve local labels, current offsets, section bases, and forward expressions.

bits 64

section .text
symbol_base:
	db 0x11
.local:
	dw .local-symbol_base
	dd $-symbol_base
	dq symbol_end-symbol_base
	times 3 db 0xaa
symbol_end:
	db $-symbol_base

