; Expected: encode RV64I loads, stores, word operations, and local control flow.

[ARCH riscv]
[BITS 64]
[INST i]

section .text
start:
	ADDI sp, sp, -16
	MOV BYT8 [sp + 8], ra
	MOV a0, BYT8 [sp + 8]
	MOV a1, BYT4 [sp]
	ADDW a0, a1
	SRAIW a0, 3
	BNE a0, zero, done
	JAL ra, done
done:
	MOV ra, BYT8 [sp + 8]
	ADDI sp, sp, 16
	RET

native_word_forms:
	ADDIW x5, x6, -4
	SLLIW x5, x6, 7
	SRLIW x5, x6, 7
	SUBW x7, x8, x9
	SLLW x7, x8, x9
	SRLW x7, x8, x9
	SRAW x7, x8, x9
	MOV x10, UNSIGNED BYT8 [sp]
