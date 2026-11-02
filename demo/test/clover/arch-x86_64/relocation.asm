; Expected: emit PC-relative code relocations and one absolute data relocation.

bits 64
default rel

global aasm_relocation_entry
extern external_function
extern external_data

section .text
aasm_relocation_entry:
	call external_function
	mov rax, [rel external_data]
	lea rdx, [rel local_data]
	ret

section .data
local_data:
	dq external_data

