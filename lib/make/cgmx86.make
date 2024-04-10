# UNISYM for MECOCOA built-2024-04-11 01:11:19.742094
.PHONY: all

unidir = /mnt/hgfs/unisym
libcdir = $(unidir)/lib/c
libadir = $(unidir)/lib/asm
asmattr = -I${unidir}/inc/Kasha/n_ -I${unidir}/inc/naasm/n_ -I./include/
asm  = /mnt/hgfs/_bin/ELF64/aasm ${asmattr} #OPT: aasm
asmf = ${asm} -felf
CC32 = gcc -m32 -c -fno-builtin -fleading-underscore -fno-pic -fno-stack-protector -I/mnt/hgfs/unisym/inc/c -D_Linux

all:
	-sudo mkdir -m 777 -p ~/_obj/libmx86
	-rm -f ~/_obj/libmx86/*.obj
	$(asmf) ./lib/asm/x86/binary.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/binary.obj
	$(asmf) ./lib/asm/x86/cpuid.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/cpuid.obj
	$(asmf) ./lib/asm/x86/disk/floppy.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/floppy.obj
	$(asmf) ./lib/asm/x86/filefmt/ELF.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/ELF.obj
	$(asmf) ./lib/asm/x86/filesys/FAT12.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/FAT12.obj
	$(asmf) ./lib/asm/x86/inst/interrupt.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/interrupt.obj
	$(asmf) ./lib/asm/x86/inst/ioport.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/ioport.obj
	$(asmf) ./lib/asm/x86/inst/manage.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/manage.obj
	$(asmf) ./lib/asm/x86/inst/stack.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/stack.obj
	$(asmf) ./lib/asm/x86/interrupt/i8259A.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/i8259A.obj
	$(asmf) ./lib/asm/x86/memory/memory.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/memory.obj
	$(asmf) ./lib/asm/x86/string/ustring.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/ustring.obj
	$(asmf) ./lib/asm/x86/toki/toki.asm -D_MCCA -Dp_i386 -o ~/_obj/libmx86/toki.obj
	$(CC32) ${libcdir}/driver/i8259A.c -D_MCCA -Dp_i386 -o ~/_obj/libmx86/i8259A.obj
	$(CC32) ${libcdir}/processor/x86/delay.c -DADDR_CountSeconds=0x524 -D_MCCA -Dp_i386 -o ~/_obj/libmx86/delay.obj
	ar -rcs /mnt/hgfs/_bin/libmx86.a ~/_obj/libmx86/*.obj
