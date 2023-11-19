ASM32DIR=../../../lib/asm/x86
DEST_BIN=../../../..
CPUID:
	aasm ${ASM32DIR}/cpuid.asm -fwin32 -o cpuid.o
	gcc chk_cpuid.c cpuid.o -m32 -o cpuid.exe
	cmd /C del cpuid.o
	cmd /C move cpuid.exe ${DEST_BIN}/_bin/