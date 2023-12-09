#Test for cpp using
ASM32DIR=../../../lib/asm/x86
DEST_BIN=../../../..
LIB=../../../../_bin
CPUID:
	g++ chk_cpuid.cpp -o cpuid.exe -L${LIB} -lw32d -O3 -D_DEBUG
	cmd /C move cpuid.exe ${DEST_BIN}/_bin/
chk_node.exe: chk_node.c
	gcc chk_node.c -L"../../../../_bin" -l"w32d" -o chk_node.exe -O3 -D_DEBUG
clean:
	cmd /C del *.exe
