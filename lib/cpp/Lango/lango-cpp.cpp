#include "../../../inc/cpp/unisym"

_ESYM_C Handler_t __init_array_start[], __init_array_end[];
_ESYM_C void _preprocess() {
	for (Handler_t* func = __init_array_start; func < __init_array_end; func++) (*func)();
}

/* RISCV
	la s0, __init_array_start
	la s1, __init_array_end
	bgeu s0, s1, .L_init_done
.L_init_loop:
	LOAD a0, 0(s0)
	addi s0, s0, SIZE_REG
	jalr ra, a0, 0
	bltu s0, s1, .L_init_loop
*/
