; Expected: encode RV32I instructions, ABI register names, AASM addresses, and aliases.

[ARCH riscv]
[BITS 32]
[CPU i]

section .text
start:
	ADDI a0, zero, 1
	ADD a0, a1
	MOV a2, a0
	MOV BYTE [sp + 3], a2
	MOV a3, BYTE [sp + 3]
	MOVU a4, BYTE [sp + 3]
	BEQ a0, a1, [done]
	J [done]
	NOP
done:
	RET

native_forms:
	LUI x1, 0x12345
	AUIPC x2, 1
	JAL x1, [native_done]
	JALR x1, [x2 + 4]
	BNE x1, x2, [native_done]
	BLT x1, x2, [native_done]
	BGE x1, x2, [native_done]
	BLTU x1, x2, [native_done]
	BGEU x1, x2, [native_done]
	LH x3, [x4 - 8]
	LW x3, [x4 - 8]
	LHU x3, [x4 - 8]
	SH x3, [x4 - 8]
	SW x3, [x4 - 8]
	SLTI x5, x6, -4
	SLTIU x5, x6, -4
	XORI x5, x6, -4
	ORI x5, x6, -4
	ANDI x5, x6, -4
	SLLI x5, x6, 7
	SRLI x5, x6, 7
	SRAI x5, x6, 7
	SUB x7, x8, x9
	SLL x7, x8, x9
	SLT x7, x8, x9
	SLTU x7, x8, x9
	XOR x7, x8, x9
	SRL x7, x8, x9
	SRA x7, x8, x9
	OR x7, x8, x9
	AND x7, x8, x9
	FENCE
	ECALL
	EBREAK
native_done:
	NOP
