gcc_o32=gcc
ChrAdd:
	aasm ../n_hstring.a -o hstring.o -fwin64
	gcc demoChrAdd.nasm.c hstring.o -o ChrAdd.exe -m64
StrLen:
	aasm ../library/x86/ustring.asm -o ustring.o -fwin32
	${gcc_o32} demoStrLength.nasm.c ustring.o -o strlen.exe -m32
clean:
	del *.o