; Expected: resolve local Binary fixups after section layout is finalized.

bits 64
[org 0x1000]

section .text progbits
binary_output_start:
	dq data_value
	lea rax, [rel data_value]
	call call_target
	jmp branch_target
	db 0x5a

section .code progbits align=16
call_target:
	ret
branch_target:
	ret

section .data progbits align=16
data_value:
	dd 0x12345678
	dq reserve_start

section .bss nobits align=16
reserve_start:
	resb 8
