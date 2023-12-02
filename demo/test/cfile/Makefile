ASM32DIR=../../../lib/asm/x86
DEST_BIN=../../../..
CPUID:
	aasm ${ASM32DIR}/cpuid.asm -fwin32 -o cpuid.o
	gcc chk_cpuid.c cpuid.o -m32 -o cpuid.exe
	cmd /C del cpuid.o
	cmd /C move cpuid.exe ${DEST_BIN}/_bin/
chk_node.exe: chk_node.c
	gcc chk_node.c -L"../../../../_bin" -l"host.cpuw32" -o chk_node.exe -O3 -D_DEBUG
clean:
	cmd /C del *.exe
