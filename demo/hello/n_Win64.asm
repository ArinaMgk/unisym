global _ento
extern ExitProcess, GetStdHandle, WriteConsoleA

section .data
	msg db "Hello, world!", 13, 10, 0
	msg.len equ $-msg

section .bss
	dummy resb 1

section .text
_ento:
	mov ecx, 0xfffffff5
	call GetStdHandle
	mov r9, dummy
	mov r8d, 0xd
	mov edx, msg
	mov rcx, rax
	call WriteConsoleA
	push 0
	call ExitProcess