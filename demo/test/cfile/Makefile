ASM32DIR=../../../lib/asm/x86
DEST_BIN=../../../..
LIB=../../../../_bin
CPUID:
	gcc chk_cpuid.c -o cpuid.exe -L${LIB} -lw32d -O3 -D_DEBUG	
	cmd /C move cpuid.exe ${DEST_BIN}/_bin/
dnode.exe: chk_dnode.c
	gcc chk_dnode.c -L"../../../../_bin" -l"w32d" -o chk_dnode.exe -O3 -D_DEBUG
	./chk_dnode.exe
	rm chk_dnode.exe
node.exe: chk_node.c
	gcc chk_node.c -L"../../../../_bin" -l"w32d" -o chk_node.exe -O3 -D_DEBUG
	./chk_node.exe
	rm chk_node.exe
crc64.exe:
	gcc chk_CRC64.c -I../../../inc/c/ -D_DEBUG -L"../../../../_bin" -lw32d
clean:
	cmd /C del *.exe
