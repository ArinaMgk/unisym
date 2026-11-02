; Expected: reject an effective address that mixes 32- and 64-bit registers.

bits 64

section .text
	mov rax, [eax+rax]

