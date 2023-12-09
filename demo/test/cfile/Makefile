ASM32DIR=../../../lib/asm/x86
DEST_BIN=../../../..
LIB=../../../../_bin
CPUID:
	gcc chk_cpuid.c -o cpuid.exe -L${LIB} -lw32d -O3 -D_DEBUG	
	cmd /C move cpuid.exe ${DEST_BIN}/_bin/
chk_node.exe: chk_node.c
	gcc chk_node.c -L"../../../../_bin" -l"w32d" -o chk_node.exe -O3 -D_DEBUG
chk_crc64.exe:
	gcc chk_CRC64.c -I../../../inc/c/ -D_DEBUG -L"../../../../_bin" -lw32d
clean:
	cmd /C del *.exe
